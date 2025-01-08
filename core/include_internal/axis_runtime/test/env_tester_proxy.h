//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/signature.h"

#define axis_ENV_TESTER_PROXY_SIGNATURE 0x12D37E14C7045A41U

typedef struct axis_env_tester_t axis_env_tester_t;

typedef struct axis_env_tester_proxy_t {
  axis_signature_t signature;

  axis_env_tester_t *axis_env_tester;
} axis_env_tester_proxy_t;
