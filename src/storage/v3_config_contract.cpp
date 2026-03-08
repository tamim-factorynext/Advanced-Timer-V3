/*
File: src/storage/v3_config_contract.cpp
Purpose: Implements the v3 config contract module behavior.

Responsibilities:
- Provide executable logic for the paired module contract.
- Keep behavior deterministic for scan-cycle/runtime execution.

Used By:
- src/storage/storage_service.cpp
Flow Hook:
- Config load/validate/normalize and persistence lifecycle.

Notes:
- Naming follows docs/naming-glossary-v3.md where applicable.
*/
#include "storage/v3_config_contract.h"

#include <string.h>

#include "platform/hw_profile.h"
#include "storage/v3_timezones.h"

namespace v3::storage {

namespace {

ConditionBlock makeInertConditionBlock(uint8_t sourceCardId) {
  ConditionBlock block = {};
  block.clauseA.sourceCardId = sourceCardId;
  block.clauseA.op = ConditionOperator::AlwaysFalse;
  block.clauseA.thresholdValue = 0;
  block.clauseA.thresholdCardId = sourceCardId;
  block.clauseA.useThresholdCard = false;
  block.clauseB.sourceCardId = sourceCardId;
  block.clauseB.op = ConditionOperator::AlwaysFalse;
  block.clauseB.thresholdValue = 0;
  block.clauseB.thresholdCardId = sourceCardId;
  block.clauseB.useThresholdCard = false;
  block.combiner = ConditionCombiner::None;
  return block;
}

void initDiCard(CardConfig& card, uint8_t id, uint8_t channel) {
  card = {};
  card.id = id;
  card.family = CardFamily::DI;
  card.enabled = true;
  card.di.channel = channel;
  card.di.invert = false;
  card.di.debounceMs = 50;
  card.di.edgeMode = 0;  // 0=RISING
  card.di.turnOnCondition = makeInertConditionBlock(id);
  card.di.turnOffCondition = makeInertConditionBlock(id);
}

void initDoCard(CardConfig& card, uint8_t id, uint8_t channel) {
  card = {};
  card.id = id;
  card.family = CardFamily::DO;
  card.enabled = true;
  card.dout.channel = channel;
  card.dout.invert = false;
  card.dout.mode = 5;  // Mode_DO_Normal
  card.dout.delayBeforeOnMs = 1000;
  card.dout.activeDurationMs = 1000;
  card.dout.repeatCount = 1;
  card.dout.turnOnCondition = makeInertConditionBlock(id);
  card.dout.turnOffCondition = makeInertConditionBlock(id);
}

void initAiCard(CardConfig& card, uint8_t id, uint8_t channel) {
  card = {};
  card.id = id;
  card.family = CardFamily::AI;
  card.enabled = true;
  card.ai.channel = channel;
  card.ai.inputMin = 0;
  card.ai.inputMax = 4095;
  card.ai.outputMin = 0;
  card.ai.outputMax = 10000;
  card.ai.smoothingFactorPct = 100;
}

void initSioCard(CardConfig& card, uint8_t id) {
  card = {};
  card.id = id;
  card.family = CardFamily::SIO;
  card.enabled = true;
  card.sio.invert = false;
  card.sio.mode = 5;  // Mode_DO_Normal
  card.sio.delayBeforeOnMs = 1000;
  card.sio.activeDurationMs = 1000;
  card.sio.repeatCount = 1;
  card.sio.turnOnCondition = makeInertConditionBlock(id);
  card.sio.turnOffCondition = makeInertConditionBlock(id);
}

void initMathCard(CardConfig& card, uint8_t id) {
  card = {};
  card.id = id;
  card.family = CardFamily::MATH;
  card.enabled = true;
  card.math.operation = 0;  // ADD
  card.math.inputAUseCard = false;
  card.math.inputACardId = id;
  card.math.inputA = 0;
  card.math.inputBUseCard = false;
  card.math.inputBCardId = id;
  card.math.inputB = 0;
  card.math.inputMin = 0;
  card.math.inputMax = 10000;
  card.math.outputMin = 0;
  card.math.outputMax = 10000;
  card.math.smoothingFactorPct = 100;
  card.math.fallbackValue = 0;
  card.math.turnOnCondition = makeInertConditionBlock(id);
  card.math.turnOffCondition = makeInertConditionBlock(id);
}

void initRtcCard(CardConfig& card, uint8_t id, uint8_t minuteOffset) {
  card = {};
  card.id = id;
  card.family = CardFamily::RTC;
  card.enabled = true;
  card.rtc.hasYear = false;
  card.rtc.year = 0;
  card.rtc.hasMonth = false;
  card.rtc.month = 0;
  card.rtc.hasDay = false;
  card.rtc.day = 0;
  card.rtc.hasWeekday = false;
  card.rtc.weekday = 0;
  card.rtc.hasHour = false;
  card.rtc.hour = 0;
  card.rtc.minute = static_cast<uint8_t>(minuteOffset % 60);
  card.rtc.triggerDurationMs = 60000;
}

}  // namespace

void makeDefaultSystemConfig(SystemConfig& cfg) {
  cfg = {};
  cfg.schemaVersion = kConfigSchemaVersion;
  cfg.scanPeriodMs = kMinScanIntervalMs;
  cfg.debugModeEnabled = false;
  strncpy(cfg.wifi.backupAccessNetwork.ssid, "advancedtimer",
          sizeof(cfg.wifi.backupAccessNetwork.ssid) - 1);
  strncpy(cfg.wifi.backupAccessNetwork.password, "12345678",
          sizeof(cfg.wifi.backupAccessNetwork.password) - 1);
  cfg.wifi.backupAccessNetwork.timeoutSec = 10;
  cfg.wifi.backupAccessNetwork.editable = false;

  strncpy(cfg.wifi.userConfiguredNetwork.ssid, "FactoryNext",
          sizeof(cfg.wifi.userConfiguredNetwork.ssid) - 1);
  strncpy(cfg.wifi.userConfiguredNetwork.password, "FactoryNext20$22#",
          sizeof(cfg.wifi.userConfiguredNetwork.password) - 1);
  cfg.wifi.userConfiguredNetwork.timeoutSec = 180;
  cfg.wifi.userConfiguredNetwork.editable = true;

  cfg.wifi.retryDelaySec = 30;
  cfg.wifi.staOnly = true;

  strncpy(cfg.time.timezone, defaultTimezoneId(), sizeof(cfg.time.timezone) - 1);
  cfg.time.timeSync.enabled = true;
  strncpy(cfg.time.timeSync.primaryTimeServer, "pool.ntp.org",
          sizeof(cfg.time.timeSync.primaryTimeServer) - 1);
  cfg.time.timeSync.syncIntervalSec = 3600;
  cfg.time.timeSync.startupTimeoutSec = 15;
  cfg.time.timeSync.maxTimeAgeSec = 86400;

  const v3::platform::HardwareProfile& profile =
      v3::platform::activeHardwareProfile();

  uint8_t nextId = 0;
  auto canAdd = [&nextId]() { return nextId < kMaxCards; };

  for (uint8_t i = 0; i < profile.diCount && canAdd(); ++i) {
    initDiCard(cfg.cards[nextId], nextId, profile.diChannels[i]);
    nextId += 1;
  }
  for (uint8_t i = 0; i < profile.doCount && canAdd(); ++i) {
    initDoCard(cfg.cards[nextId], nextId, profile.doChannels[i]);
    nextId += 1;
  }
  for (uint8_t i = 0; i < profile.aiCount && canAdd(); ++i) {
    initAiCard(cfg.cards[nextId], nextId, profile.aiChannels[i]);
    nextId += 1;
  }
  for (uint8_t i = 0; i < profile.sioCapacity && canAdd(); ++i) {
    initSioCard(cfg.cards[nextId], nextId);
    nextId += 1;
  }
  for (uint8_t i = 0; i < profile.mathCapacity && canAdd(); ++i) {
    initMathCard(cfg.cards[nextId], nextId);
    nextId += 1;
  }
  for (uint8_t i = 0; i < profile.rtcCapacity && canAdd(); ++i) {
    initRtcCard(cfg.cards[nextId], nextId, i);
    nextId += 1;
  }

  cfg.cardCount = nextId;
}

SystemConfig makeDefaultSystemConfig() {
  SystemConfig cfg = {};
  makeDefaultSystemConfig(cfg);
  return cfg;
}

}  // namespace v3::storage



