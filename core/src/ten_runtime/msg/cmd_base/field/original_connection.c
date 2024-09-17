//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/msg/cmd_base/field/original_connection.h"

#include "include_internal/ten_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/mark.h"

void ten_cmd_base_copy_original_connection(
    ten_msg_t *self, ten_msg_t *src,
    TEN_UNUSED ten_list_t *excluded_field_ids) {
  TEN_ASSERT(src && ten_raw_msg_check_integrity(src), "Should not happen.");

  ((ten_cmd_base_t *)self)->original_connection =
      ((ten_cmd_base_t *)src)->original_connection;
}
