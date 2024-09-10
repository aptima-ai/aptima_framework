//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ten_utils/ten_config.h"

typedef struct ten_log_src_location_t {
  const char *func_name;
  const char *file_name;
  const size_t line;
} ten_log_src_location_t;

typedef enum TEN_SIGN {
  TEN_SIGN_NEGATIVE = -1,
  TEN_SIGN_ZERO = 0,
  TEN_SIGN_POSITIVE = 1
} TEN_SIGN;
