//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timer/cmd.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/cmd.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd/timer/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_utils/value/value_path.h"
#include "include_internal/axis_utils/value/value_set.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"
#include "axis_utils/value/value_is.h"

static axis_cmd_timer_t *get_raw_cmd(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_cmd_base_check_integrity(self), "Should not happen.");

  return (axis_cmd_timer_t *)axis_shared_ptr_get_data(self);
}

bool axis_raw_cmd_timer_set_timer_id(axis_cmd_timer_t *self, uint32_t timer_id) {
  axis_ASSERT(
      self && axis_raw_msg_get_type((axis_msg_t *)self) == axis_MSG_TYPE_CMD_TIMER,
      "Should not happen.");

  return axis_value_set_uint32(&self->timer_id, timer_id);
}

bool axis_raw_cmd_timer_set_times(axis_cmd_timer_t *self, int32_t times) {
  axis_ASSERT(
      self && axis_raw_msg_get_type((axis_msg_t *)self) == axis_MSG_TYPE_CMD_TIMER,
      "Should not happen.");

  return axis_value_set_int32(&self->times, times);
}

static void axis_raw_cmd_timer_destroy(axis_cmd_timer_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_deinit(&self->cmd_hdr);

  axis_value_deinit(&self->timer_id);
  axis_value_deinit(&self->timeout_in_us);
  axis_value_deinit(&self->times);

  axis_FREE(self);
}

void axis_raw_cmd_timer_as_msg_destroy(axis_msg_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_cmd_timer_destroy((axis_cmd_timer_t *)self);
}

bool axis_raw_cmd_timer_loop_all_fields(axis_msg_t *self,
                                       axis_raw_msg_process_one_field_func_t cb,
                                       void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  for (size_t i = 0; i < axis_cmd_timer_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_cmd_timer_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}

axis_cmd_timer_t *axis_raw_cmd_timer_create(void) {
  axis_cmd_timer_t *raw_cmd = axis_MALLOC(sizeof(axis_cmd_timer_t));
  axis_ASSERT(raw_cmd, "Failed to allocate memory.");

  axis_raw_cmd_init(&raw_cmd->cmd_hdr, axis_MSG_TYPE_CMD_TIMER);

  axis_value_init_uint32(&raw_cmd->timer_id, 0);
  axis_value_init_uint64(&raw_cmd->timeout_in_us, 0);
  axis_value_init_int32(&raw_cmd->times, 0);

  return raw_cmd;
}

axis_shared_ptr_t *axis_cmd_timer_create(void) {
  return axis_shared_ptr_create(axis_raw_cmd_timer_create(),
                               axis_raw_cmd_timer_destroy);
}

uint32_t axis_raw_cmd_timer_get_timer_id(axis_cmd_timer_t *self) {
  axis_ASSERT(
      self && axis_raw_msg_get_type((axis_msg_t *)self) == axis_MSG_TYPE_CMD_TIMER,
      "Should not happen.");

  return axis_value_get_uint32(&self->timer_id, NULL);
}

uint32_t axis_cmd_timer_get_timer_id(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_TIMER,
             "Should not happen.");

  return axis_raw_cmd_timer_get_timer_id(get_raw_cmd(self));
}

uint64_t axis_raw_cmd_timer_get_timeout_in_us(axis_cmd_timer_t *self) {
  axis_ASSERT(
      self && axis_raw_msg_get_type((axis_msg_t *)self) == axis_MSG_TYPE_CMD_TIMER,
      "Should not happen.");

  return axis_value_get_uint64(&self->timeout_in_us, NULL);
}

uint64_t axis_cmd_timer_get_timeout_in_us(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_TIMER,
             "Should not happen.");

  return axis_raw_cmd_timer_get_timeout_in_us(get_raw_cmd(self));
}

static bool axis_raw_cmd_timer_set_timeout_in_us(axis_cmd_timer_t *self,
                                                uint64_t timeout_in_us) {
  axis_ASSERT(
      self && axis_raw_msg_get_type((axis_msg_t *)self) == axis_MSG_TYPE_CMD_TIMER,
      "Should not happen.");

  return axis_value_set_uint64(&self->timeout_in_us, timeout_in_us);
}

bool axis_cmd_timer_set_timeout_in_us(axis_shared_ptr_t *self,
                                     uint64_t timeout_in_us) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_TIMER,
             "Should not happen.");

  return axis_raw_cmd_timer_set_timeout_in_us(get_raw_cmd(self), timeout_in_us);
}

int32_t axis_raw_cmd_timer_get_times(axis_cmd_timer_t *self) {
  axis_ASSERT(
      self && axis_raw_msg_get_type((axis_msg_t *)self) == axis_MSG_TYPE_CMD_TIMER,
      "Should not happen.");

  return axis_value_get_int32(&self->times, NULL);
}

bool axis_raw_cmd_timer_set_axis_property(axis_msg_t *self, axis_list_t *paths,
                                        axis_value_t *value, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Invalid argument.");
  axis_ASSERT(paths && axis_list_check_integrity(paths),
             "path should not be empty.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Invalid argument.");

  bool success = true;

  axis_error_t tmp_err;
  bool use_tmp_err = false;
  if (!err) {
    use_tmp_err = true;
    axis_error_init(&tmp_err);
    err = &tmp_err;
  }

  axis_cmd_timer_t *timer_cmd = (axis_cmd_timer_t *)self;

  axis_list_foreach (paths, item_iter) {
    axis_value_path_item_t *item = axis_ptr_listnode_get(item_iter.node);
    axis_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if (!strcmp(axis_STR_TIMER_ID,
                    axis_string_get_raw_str(&item->obj_item_str))) {
          axis_value_set_uint32(&timer_cmd->timer_id,
                               axis_value_get_uint32(value, err));
          success = axis_error_is_success(err);
        } else if (!strcmp(axis_STR_TIMEOUT_IN_US,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          axis_value_set_uint64(&timer_cmd->timeout_in_us,
                               axis_value_get_uint64(value, err));
          success = axis_error_is_success(err);
        } else if (!strcmp(axis_STR_TIMES,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          axis_value_set_int32(&timer_cmd->times,
                              axis_value_get_int32(value, err));
          success = axis_error_is_success(err);
        } else if (!strcmp(axis_STR_NAME,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          if (axis_value_is_string(value)) {
            axis_value_set_string_with_size(
                &self->name, axis_value_peek_raw_str(value, &tmp_err),
                strlen(axis_value_peek_raw_str(value, &tmp_err)));
            success = true;
          } else {
            success = false;
          }
        }
        break;
      }

      default:
        break;
    }
  }

  if (use_tmp_err) {
    axis_error_deinit(&tmp_err);
  }

  return success;
}

int32_t axis_cmd_timer_get_times(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_msg_get_type(self) == axis_MSG_TYPE_CMD_TIMER,
             "Should not happen.");

  return axis_raw_cmd_timer_get_times(get_raw_cmd(self));
}

bool axis_cmd_timer_set_timer_id(axis_shared_ptr_t *self, uint32_t timer_id) {
  return axis_raw_cmd_timer_set_timer_id(get_raw_cmd(self), timer_id);
}

bool axis_cmd_timer_set_times(axis_shared_ptr_t *self, int32_t times) {
  return axis_raw_cmd_timer_set_times(get_raw_cmd(self), times);
}
