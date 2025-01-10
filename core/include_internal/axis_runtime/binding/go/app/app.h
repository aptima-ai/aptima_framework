//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "src/axis_runtime/binding/go/interface/aptima/common.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/lib/signature.h"

#define axis_GO_APP_SIGNATURE 0xB97676170237FB01U

typedef struct axis_go_app_t {
  axis_signature_t signature;

  axis_go_bridge_t bridge;

  axis_app_t *c_app;
} axis_go_app_t;

axis_RUNTIME_PRIVATE_API bool axis_go_app_check_integrity(axis_go_app_t *self);
