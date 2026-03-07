#pragma once

#include <stddef.h>

namespace v3::storage {

struct TimezoneOption {
  const char* id;
  const char* label;
  const char* posixTz;
};

const TimezoneOption* timezoneOptions(size_t& outCount);
const TimezoneOption* findTimezoneOption(const char* timezoneId);
const TimezoneOption* canonicalTimezoneOption(const char* timezoneId);
bool isSupportedTimezone(const char* timezoneId);
const char* resolvePosixTimezone(const char* timezoneId);
const char* defaultTimezoneId();

}  // namespace v3::storage
