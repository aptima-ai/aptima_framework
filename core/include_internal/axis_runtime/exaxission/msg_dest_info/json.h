//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/json.h"

typedef struct axis_msg_dest_info_t axis_msg_dest_info_t;
typedef struct axis_extension_info_t axis_extension_info_t;

axis_RUNTIME_PRIVATE_API axis_json_t *axis_msg_dest_info_to_json(
    axis_msg_dest_info_t *self, axis_extension_info_t *src_extension_info,
    axis_error_t *err);
