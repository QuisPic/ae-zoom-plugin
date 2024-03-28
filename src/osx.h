#pragma once

#include "AEConfig.h"

#ifdef AE_OS_MAC
#include <optional>

namespace osx {
int getFrontmostAppPID();
std::optional<int> windowUnderCursorPID();
void addEventMonitor();
} // namespace osx
#endif
