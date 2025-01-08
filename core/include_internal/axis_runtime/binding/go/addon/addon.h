//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "src/axis_runtime/binding/go/interface/ten/common.h"
#include "axis_runtime/addon/addon.h"
#include "axis_utils/lib/signature.h"

#define axis_GO_ADDON_SIGNATURE 0x00FCE9927FA352FBU

typedef struct axis_go_addon_t {
  axis_signature_t signature;

  axis_go_bridge_t bridge;

  axis_addon_t c_addon;

  axis_ADDON_TYPE type;

  axis_string_t addon_name;
} axis_go_addon_t;

axis_RUNTIME_PRIVATE_API bool axis_go_addon_check_integrity(axis_go_addon_t *self);

axis_RUNTIME_PRIVATE_API axis_go_handle_t
axis_go_addon_go_handle(axis_go_addon_t *self);
