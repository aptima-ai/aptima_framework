//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/value/value.h"

typedef struct axis_extension_info_t axis_extension_info_t;

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_extension_info_node_from_value(
    axis_value_t *value, axis_list_t *extensions_info, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *
axis_extension_info_parse_connection_src_part_from_value(
    axis_value_t *value, axis_list_t *extensions_info, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *
axis_extension_info_parse_connection_dest_part_from_value(
    axis_value_t *value, axis_list_t *extensions_info,
    axis_extension_info_t *src_extension_info, const char *origin_cmd_name,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_extension_info_node_to_value(
    axis_extension_info_t *self, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_extension_info_connection_to_value(
    axis_extension_info_t *self, axis_error_t *err);
