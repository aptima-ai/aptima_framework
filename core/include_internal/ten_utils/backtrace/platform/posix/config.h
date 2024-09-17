//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#if defined(OS_LINUX) || defined(OS_ANDROID)

#include "include_internal/ten_utils/backtrace/platform/posix/linux/config_linux.h"

#elif defined(OS_MACOS)

#include "include_internal/ten_utils/backtrace/platform/posix/darwin/config_mac.h"

#endif
