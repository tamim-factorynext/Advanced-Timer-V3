#pragma once

#include <stdint.h>

namespace at {
namespace core {

class CoreService {
 public:
  virtual ~CoreService() = default;
  virtual const char *name() const = 0;
  virtual void init() = 0;
  virtual void tick(uint32_t nowMs) = 0;
};

}  // namespace core
}  // namespace at

