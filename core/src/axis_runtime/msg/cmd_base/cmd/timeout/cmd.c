//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timeout/cmd.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timeout/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_utils/value/value_set.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"

static axis_cmd_timeout_t *get_raw_cmd(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");
  return (axis_cmd_timeout_t *)axis_shared_ptr_get_data(self);
}

void axis_raw_cmd_timeout_set_timer_id(axis_cmd_timeout_t *self,
                                      uint32_t timer_id) {
  axis_ASSERT(
      self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) &&
          axis_raw_msg_get_type((axis_msg_t *)self) == axis_MSG_TYPE_CMD_TIMEOUT,
      "Should not happen.");

  axis_value_set_uint32(&self->timer_id, timer_id);
}

static void axis_raw_cmd_timeout_destroy(axis_cmd_timeout_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_deinit(&self->cmd_hdr);

  axis_value_deinit(&self->timer_id);
  axis_FREE(self);
}

void axis_raw_cmd_timeout_as_msg_destroy(axis_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_timeout_destroy((axis_cmd_timeout_t *)self);
}

static axis_cmd_timeout_t *axis_raw_cmd_timeout_create(const uint32_t timer_id) {
  axis_cmd_timeout_t *raw_cmd = axis_MALLOC(sizeof(axis_cmd_timeout_t));
  axis_ASSERT(raw_cmd, "Failed to allocate memory.");

  axis_raw_cmd_init(&raw_cmd->cmd_hdr, axis_MSG_TYPE_CMD_TIMEOUT);
  axis_value_init_uint32(&raw_cmd->timer_id, timer_id);

  return raw_cmd;
}

axis_shared_ptr_t *axis_cmd_timeout_create(const uint32_t timer_id) {
  return axis_shared_ptr_create(axis_raw_cmd_timeout_create(timer_id),
                               axis_raw_cmd_timeout_destroy);
}

uint32_t axis_raw_cmd_timeout_get_timer_id(axis_cmd_timeout_t *self) {
  axis_ASSERT(
      self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) &&
          axis_raw_msg_get_type((axis_msg_t *)self) == axis_MSG_TYPE_CMD_TIMEOUT,
      "Should not happen.");

  return axis_value_get_uint32(&self->timer_id, NULL);
}

uint32_t axis_cmd_timeout_get_timer_id(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self) &&
                 axis_msg_get_type(self) == axis_MSG_TYPE_CMD_TIMEOUT,
             "Should not happen.");

  return axis_raw_cmd_timeout_get_timer_id(get_raw_cmd(self));
}

void axis_cmd_timeout_set_timer_id(axis_shared_ptr_t *self, uint32_t timer_id) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_timeout_set_timer_id(get_raw_cmd(self), timer_id);
}

bool axis_raw_cmd_timeout_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(self && axis_raw_cmd_check_integrity((axis_cmd_t *)self) && cb,
             "Should not happen.");

  for (size_t i = 0; i < axis_cmd_timeout_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_cmd_timeout_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}
