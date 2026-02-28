#pragma once

#include <stdint.h>

enum logicCardType { DigitalInput, DigitalOutput, AnalogInput, SoftIO };

enum logicOperator {
  Op_AlwaysTrue,
  Op_AlwaysFalse,
  Op_LogicalTrue,
  Op_LogicalFalse,
  Op_PhysicalOn,
  Op_PhysicalOff,
  Op_Triggered,
  Op_TriggerCleared,
  Op_GT,
  Op_LT,
  Op_EQ,
  Op_NEQ,
  Op_GTE,
  Op_LTE,
  Op_Running,
  Op_Finished,
  Op_Stopped
};

enum cardMode {
  Mode_None,
  Mode_DI_Rising,
  Mode_DI_Falling,
  Mode_DI_Change,
  Mode_AI_Continuous,
  Mode_DO_Normal,
  Mode_DO_Immediate,
  Mode_DO_Gated
};

enum cardState {
  State_None,
  State_DI_Idle,
  State_DI_Filtering,
  State_DI_Qualified,
  State_DI_Inhibited,
  State_AI_Streaming,
  State_DO_Idle,
  State_DO_OnDelay,
  State_DO_Active,
  State_DO_Finished
};

enum combineMode { Combine_None, Combine_AND, Combine_OR };

struct LogicCard {
  uint8_t id;
  logicCardType type;
  uint8_t index;
  uint8_t hwPin;
  bool invert;
  uint32_t setting1;
  uint32_t setting2;
  uint32_t setting3;
  bool logicalState;
  bool physicalState;
  bool triggerFlag;
  uint32_t currentValue;
  uint32_t startOnMs;
  uint32_t startOffMs;
  uint32_t repeatCounter;
  cardMode mode;
  cardState state;
  uint8_t setA_ID;
  logicOperator setA_Operator;
  uint32_t setA_Threshold;
  uint8_t setB_ID;
  logicOperator setB_Operator;
  uint32_t setB_Threshold;
  combineMode setCombine;
  uint8_t resetA_ID;
  logicOperator resetA_Operator;
  uint32_t resetA_Threshold;
  uint8_t resetB_ID;
  logicOperator resetB_Operator;
  uint32_t resetB_Threshold;
  combineMode resetCombine;
};
