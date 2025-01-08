//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "include_internal/axis_runtime/test/env_tester.h"

typedef struct axis_go_axis_env_tester_t {
  axis_signature_t signature;

  axis_go_bridge_t bridge;

  axis_env_tester_t *c_axis_env_tester;
} axis_go_axis_env_tester_t;

axis_RUNTIME_PRIVATE_API bool axis_go_axis_env_tester_check_integrity(
    axis_go_axis_env_tester_t *self);

axis_RUNTIME_PRIVATE_API axis_go_axis_env_tester_t *
axis_go_axis_env_tester_reinterpret(uintptr_t bridge_addr);

axis_RUNTIME_PRIVATE_API axis_go_axis_env_tester_t *axis_go_axis_env_tester_wrap(
    axis_env_tester_t *c_axis_env_tester);

axis_RUNTIME_PRIVATE_API axis_go_handle_t
axis_go_axis_env_tester_go_handle(axis_go_axis_env_tester_t *self);

axis_RUNTIME_API axis_go_handle_t tenGoCreateTenEnvTester(uintptr_t);
