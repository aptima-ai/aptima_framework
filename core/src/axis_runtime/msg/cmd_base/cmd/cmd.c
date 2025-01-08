//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"

#include <sys/types.h>

#include "include_internal/axis_runtime/msg/cmd_base/cmd/custom/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/field/field_info.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/stop_graph/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg/msg_info.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/cmd/cmd.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

bool axis_raw_cmd_check_integrity(axis_cmd_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_CMD_SIGNATURE) {
    return false;
  }

  if (!axis_raw_msg_is_cmd(&self->cmd_base_hdr.msg_hdr)) {
    return false;
  }

  return true;
}

static axis_cmd_t *axis_cmd_get_raw_cmd(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return (axis_cmd_t *)axis_shared_ptr_get_data(self);
}

bool axis_cmd_check_integrity(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_raw_cmd_check_integrity(axis_cmd_get_raw_cmd(self)) == false) {
    return false;
  }
  return true;
}

void axis_raw_cmd_init(axis_cmd_t *self, axis_MSG_TYPE type) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_base_init(&self->cmd_base_hdr, type);
  axis_signature_set(&self->signature, (axis_signature_t)axis_CMD_SIGNATURE);
}

void axis_raw_cmd_deinit(axis_cmd_t *self) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity(self), "Should not happen.");

  axis_signature_set(&self->signature, 0);

  axis_raw_cmd_base_deinit(&self->cmd_base_hdr);
}

void axis_raw_cmd_copy_field(axis_msg_t *self, axis_msg_t *src,
                            axis_list_t *excluded_field_ids) {
  axis_ASSERT(src && axis_raw_cmd_check_integrity((axis_cmd_t *)src) && self,
             "Should not happen.");

  for (size_t i = 0; i < axis_cmd_fields_info_size; ++i) {
    if (excluded_field_ids) {
      bool skip = false;

      axis_list_foreach (excluded_field_ids, iter) {
        if (axis_cmd_fields_info[i].field_id ==
            axis_int32_listnode_get(iter.node)) {
          skip = true;
          break;
        }
      }

      if (skip) {
        continue;
      }
    }

    axis_msg_copy_field_func_t copy_field = axis_cmd_fields_info[i].copy_field;
    if (copy_field) {
      copy_field(self, src, excluded_field_ids);
    }
  }
}

bool axis_raw_cmd_process_field(axis_msg_t *self,
                               axis_raw_msg_process_one_field_func_t cb,
                               void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) && cb,
             "Should not happen.");

  for (size_t i = 0; i < axis_cmd_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_cmd_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}

void axis_raw_cmd_destroy(axis_cmd_t *self) {
  axis_ASSERT(self, "Should not happen.");

  switch (self->cmd_base_hdr.msg_hdr.type) {
    case axis_MSG_TYPE_CMD:
      axis_raw_cmd_custom_as_msg_destroy((axis_msg_t *)self);
      break;
    case axis_MSG_TYPE_CMD_STOP_GRAPH:
      axis_raw_cmd_stop_graph_as_msg_destroy((axis_msg_t *)self);
      break;
    case axis_MSG_TYPE_CMD_CLOSE_APP:
      axis_raw_cmd_close_app_as_msg_destroy((axis_msg_t *)self);
      break;
    case axis_MSG_TYPE_CMD_TIMEOUT:
      axis_raw_cmd_timeout_as_msg_destroy((axis_msg_t *)self);
      break;
    case axis_MSG_TYPE_CMD_TIMER:
      axis_raw_cmd_timer_as_msg_destroy((axis_msg_t *)self);
      break;
    case axis_MSG_TYPE_CMD_START_GRAPH:
      axis_raw_cmd_start_graph_as_msg_destroy((axis_msg_t *)self);
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}

static axis_MSG_TYPE axis_cmd_type_from_name_string(const char *name_str) {
  axis_MSG_TYPE msg_type = axis_MSG_TYPE_CMD;

  for (size_t i = 0; i < axis_msg_info_size; i++) {
    if (axis_msg_info[i].msg_unique_name &&
        axis_c_string_is_equal(name_str, axis_msg_info[i].msg_unique_name)) {
      msg_type = (axis_MSG_TYPE)i;
      break;
    }
  }

  switch (msg_type) {
    case axis_MSG_TYPE_INVALID:
    case axis_MSG_TYPE_DATA:
    case axis_MSG_TYPE_VIDEO_FRAME:
    case axis_MSG_TYPE_AUDIO_FRAME:
    case axis_MSG_TYPE_CMD_RESULT:
      return axis_MSG_TYPE_INVALID;

    default:
      return msg_type;
  }
}

static axis_cmd_t *axis_raw_cmd_create(const char *name, axis_error_t *err) {
  if (!name) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC,
                    "Failed to create cmd without a name.");
    }
    return NULL;
  }

  axis_MSG_TYPE cmd_type = axis_cmd_type_from_name_string(name);

  switch (cmd_type) {
    case axis_MSG_TYPE_CMD:
      return axis_raw_cmd_custom_create(name, err);
    case axis_MSG_TYPE_CMD_STOP_GRAPH:
      return (axis_cmd_t *)axis_raw_cmd_stop_graph_create();
    case axis_MSG_TYPE_CMD_CLOSE_APP:
      return (axis_cmd_t *)axis_raw_cmd_close_app_create();
    case axis_MSG_TYPE_CMD_TIMER:
      return (axis_cmd_t *)axis_raw_cmd_timer_create();
    case axis_MSG_TYPE_CMD_START_GRAPH:
      return (axis_cmd_t *)axis_raw_cmd_start_graph_create();

    default:
      axis_ASSERT(0, "Should not happen.");
      return NULL;
  }
}

axis_shared_ptr_t *axis_cmd_create(const char *name, axis_error_t *err) {
  axis_cmd_t *cmd = axis_raw_cmd_create(name, err);
  if (!cmd) {
    return NULL;
  }
  return axis_shared_ptr_create(cmd, axis_raw_cmd_destroy);
}
