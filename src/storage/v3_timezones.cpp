#include "storage/v3_timezones.h"

#include <cstring>

namespace v3::storage {

namespace {

constexpr TimezoneOption kTimezoneOptions[] = {
    {"UTC-12:00", "(UTC-12:00) Baker Island", "UTC12"},
    {"UTC-11:00", "(UTC-11:00) American Samoa", "UTC11"},
    {"UTC-10:00", "(UTC-10:00) Hawaii", "UTC10"},
    {"UTC-09:30", "(UTC-09:30) Marquesas", "UTC9:30"},
    {"UTC-09:00", "(UTC-09:00) Alaska", "UTC9"},
    {"UTC-08:00", "(UTC-08:00) Pacific Time", "UTC8"},
    {"UTC-07:00", "(UTC-07:00) Mountain Time", "UTC7"},
    {"UTC-06:00", "(UTC-06:00) Central Time", "UTC6"},
    {"UTC-05:00", "(UTC-05:00) Eastern Time", "UTC5"},
    {"UTC-04:00", "(UTC-04:00) Atlantic Time", "UTC4"},
    {"UTC-03:30", "(UTC-03:30) Newfoundland", "UTC3:30"},
    {"UTC-03:00", "(UTC-03:00) Argentina", "UTC3"},
    {"UTC-02:00", "(UTC-02:00) South Georgia", "UTC2"},
    {"UTC-01:00", "(UTC-01:00) Azores", "UTC1"},
    {"UTC+00:00", "(UTC+00:00) UTC", "UTC0"},
    {"UTC+01:00", "(UTC+01:00) Central Europe", "UTC-1"},
    {"UTC+02:00", "(UTC+02:00) Eastern Europe", "UTC-2"},
    {"UTC+03:00", "(UTC+03:00) East Africa", "UTC-3"},
    {"UTC+03:30", "(UTC+03:30) Iran", "UTC-3:30"},
    {"UTC+04:00", "(UTC+04:00) Gulf", "UTC-4"},
    {"UTC+04:30", "(UTC+04:30) Afghanistan", "UTC-4:30"},
    {"UTC+05:00", "(UTC+05:00) Pakistan", "UTC-5"},
    {"UTC+05:30", "(UTC+05:30) India", "UTC-5:30"},
    {"UTC+05:45", "(UTC+05:45) Nepal", "UTC-5:45"},
    {"UTC+06:00", "(UTC+06:00) Bangladesh", "UTC-6"},
    {"UTC+06:30", "(UTC+06:30) Myanmar", "UTC-6:30"},
    {"UTC+07:00", "(UTC+07:00) Indochina", "UTC-7"},
    {"UTC+08:00", "(UTC+08:00) China / Singapore", "UTC-8"},
    {"UTC+08:45", "(UTC+08:45) Eucla", "UTC-8:45"},
    {"UTC+09:00", "(UTC+09:00) Japan / Korea", "UTC-9"},
    {"UTC+09:30", "(UTC+09:30) Central Australia", "UTC-9:30"},
    {"UTC+10:00", "(UTC+10:00) Eastern Australia", "UTC-10"},
    {"UTC+10:30", "(UTC+10:30) Lord Howe", "UTC-10:30"},
    {"UTC+11:00", "(UTC+11:00) Solomon Islands", "UTC-11"},
    {"UTC+12:00", "(UTC+12:00) New Zealand", "UTC-12"},
    {"UTC+12:45", "(UTC+12:45) Chatham Islands", "UTC-12:45"},
    {"UTC+13:00", "(UTC+13:00) Tonga", "UTC-13"},
    {"UTC+14:00", "(UTC+14:00) Line Islands", "UTC-14"},
};

struct TimezoneAlias {
  const char* alias;
  const char* canonicalId;
};

constexpr TimezoneAlias kTimezoneAliases[] = {
    {"Asia/Dhaka", "UTC+06:00"},
    {"UTC0", "UTC+00:00"},
    {"UTC", "UTC+00:00"},
};

}  // namespace

const TimezoneOption* timezoneOptions(size_t& outCount) {
  outCount = sizeof(kTimezoneOptions) / sizeof(kTimezoneOptions[0]);
  return kTimezoneOptions;
}

const TimezoneOption* findTimezoneOption(const char* timezoneId) {
  if (timezoneId == nullptr || timezoneId[0] == '\0') return nullptr;
  for (const TimezoneOption& option : kTimezoneOptions) {
    if (std::strcmp(option.id, timezoneId) == 0) return &option;
  }
  return nullptr;
}

const TimezoneOption* canonicalTimezoneOption(const char* timezoneId) {
  if (const TimezoneOption* direct = findTimezoneOption(timezoneId)) {
    return direct;
  }
  if (timezoneId == nullptr || timezoneId[0] == '\0') return nullptr;
  for (const TimezoneAlias& alias : kTimezoneAliases) {
    if (std::strcmp(alias.alias, timezoneId) == 0) {
      return findTimezoneOption(alias.canonicalId);
    }
  }
  return nullptr;
}

bool isSupportedTimezone(const char* timezoneId) {
  return canonicalTimezoneOption(timezoneId) != nullptr;
}

const char* resolvePosixTimezone(const char* timezoneId) {
  const TimezoneOption* option = canonicalTimezoneOption(timezoneId);
  return option != nullptr ? option->posixTz : "UTC0";
}

const char* defaultTimezoneId() { return "UTC+06:00"; }

}  // namespace v3::storage
