//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/cmd_base/field/original_connection.h"

#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

void axis_cmd_base_copy_original_connection(
    axis_msg_t *self, axis_msg_t *src,
    axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(src && axis_raw_msg_check_integrity(src), "Should not happen.");

  ((axis_cmd_base_t *)self)->original_connection =
      ((axis_cmd_base_t *)src)->original_connection;
}
