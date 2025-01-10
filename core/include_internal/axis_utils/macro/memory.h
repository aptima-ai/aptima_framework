//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "include_internal/axis_utils/sanitizer/memory_check.h"  // IWYU pragma: keep

#if defined(axis_ENABLE_MEMORY_CHECK)
#define axis_STRNDUP(str, size) \
  axis_sanitizer_memory_strndup((str), (size), __FILE__, __LINE__, __FUNCTION__)
#else

#define axis_STRNDUP(str, size) axis_strndup((str), (size))
#endif
