//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/msg/data/data.h"

#include "include_internal/axis_runtime/msg/data/data.h"
#include "include_internal/axis_runtime/msg/data/field/field_info.h"
#include "include_internal/axis_runtime/msg/locked_res.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_utils/value/value_path.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"

bool axis_raw_data_check_integrity(axis_data_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_DATA_SIGNATURE) {
    return false;
  }

  if (self->msg_hdr.type != axis_MSG_TYPE_DATA) {
    return false;
  }

  return true;
}

static axis_data_t *axis_raw_data_create_empty(void) {
  axis_data_t *self = (axis_data_t *)axis_MALLOC(sizeof(axis_data_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_raw_msg_init(&self->msg_hdr, axis_MSG_TYPE_DATA);
  axis_signature_set(&self->signature, axis_DATA_SIGNATURE);
  axis_value_init_buf(&self->buf, 0);

  return self;
}

axis_shared_ptr_t *axis_data_create_empty(void) {
  axis_data_t *self = axis_raw_data_create_empty();
  return axis_shared_ptr_create(self, axis_raw_data_destroy);
}

static axis_data_t *axis_raw_data_create(const char *name, axis_error_t *err) {
  axis_data_t *self = axis_raw_data_create_empty();
  axis_raw_msg_set_name((axis_msg_t *)self, name, err);
  return self;
}

static axis_data_t *axis_raw_data_create_with_name_len(const char *name,
                                                     size_t name_len,
                                                     axis_error_t *err) {
  axis_data_t *self = axis_raw_data_create_empty();
  axis_raw_msg_set_name_with_len((axis_msg_t *)self, name, name_len, err);
  return self;
}

void axis_raw_data_destroy(axis_data_t *self) {
  axis_ASSERT(self && axis_raw_data_check_integrity(self), "Should not happen.");

  axis_raw_msg_deinit(&self->msg_hdr);
  axis_value_deinit(&self->buf);

  axis_FREE(self);
}

axis_shared_ptr_t *axis_data_create(const char *name, axis_error_t *err) {
  axis_ASSERT(name, "Invalid argument.");
  axis_data_t *self = axis_raw_data_create(name, err);
  return axis_shared_ptr_create(self, axis_raw_data_destroy);
}

axis_shared_ptr_t *axis_data_create_with_name_len(const char *name,
                                                size_t name_len,
                                                axis_error_t *err) {
  axis_ASSERT(name, "Invalid argument.");
  axis_data_t *self = axis_raw_data_create_with_name_len(name, name_len, err);
  return axis_shared_ptr_create(self, axis_raw_data_destroy);
}

static void axis_raw_data_set_buf_with_move(axis_data_t *self, axis_buf_t *buf) {
  axis_ASSERT(self && axis_raw_data_check_integrity(self), "Should not happen.");
  axis_buf_move(axis_value_peek_buf(&self->buf), buf);
}

static axis_buf_t *axis_raw_data_peek_buf(axis_data_t *self) {
  axis_ASSERT(self && axis_raw_data_check_integrity(self), "Should not happen.");
  return axis_value_peek_buf(&self->buf);
}

axis_buf_t *axis_data_peek_buf(axis_shared_ptr_t *self_) {
  axis_ASSERT(self_, "Should not happen.");
  axis_data_t *self = axis_shared_ptr_get_data(self_);
  return axis_raw_data_peek_buf(self);
}

void axis_data_set_buf_with_move(axis_shared_ptr_t *self_, axis_buf_t *buf) {
  axis_ASSERT(self_, "Should not happen.");

  axis_data_t *self = axis_shared_ptr_get_data(self_);
  axis_raw_data_set_buf_with_move(self, buf);
}

void axis_raw_data_buf_copy(axis_msg_t *self, axis_msg_t *src,
                           axis_UNUSED axis_list_t *excluded_field_ids) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(src, "Invalid argument.");

  axis_data_t *src_data = (axis_data_t *)src;
  axis_data_t *self_data = (axis_data_t *)self;

  if (axis_buf_get_size(axis_value_peek_buf(&src_data->buf))) {
    axis_buf_init_with_copying_data(axis_value_peek_buf(&self_data->buf),
                                   axis_value_peek_buf(&src_data->buf)->data,
                                   axis_value_peek_buf(&src_data->buf)->size);
  }
}

axis_msg_t *axis_raw_data_as_msg_clone(axis_msg_t *self,
                                     axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self) &&
                 axis_raw_msg_get_type(self) == axis_MSG_TYPE_DATA,
             "Should not happen.");

  axis_data_t *new_data = axis_raw_data_create_empty();
  axis_ASSERT(new_data, "Failed to allocate memory.");

  for (size_t i = 0; i < axis_data_fields_info_size; ++i) {
    if (excluded_field_ids) {
      bool skip = false;

      axis_list_foreach (excluded_field_ids, iter) {
        if (axis_data_fields_info[i].field_id ==
            axis_int32_listnode_get(iter.node)) {
          skip = true;
          break;
        }
      }

      if (skip) {
        continue;
      }
    }

    axis_msg_copy_field_func_t copy_field = axis_data_fields_info[i].copy_field;
    if (copy_field) {
      copy_field((axis_msg_t *)new_data, self, excluded_field_ids);
    }
  }

  return (axis_msg_t *)new_data;
}

bool axis_raw_data_loop_all_fields(axis_msg_t *self,
                                  axis_raw_msg_process_one_field_func_t cb,
                                  axis_UNUSED void *user_data,
                                  axis_error_t *err) {
  for (size_t i = 0; i < axis_data_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_data_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}

static uint8_t *axis_raw_data_alloc_buf(axis_data_t *self, size_t size) {
  uint8_t *data = axis_value_peek_buf(&self->buf)->data;
  if (data) {
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  if (!axis_value_init_buf(&self->buf, size)) {
    return NULL;
  }

  return axis_value_peek_buf(&self->buf)->data;
}

uint8_t *axis_data_alloc_buf(axis_shared_ptr_t *self, size_t size) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_data_alloc_buf(axis_shared_ptr_get_data(self), size);
}

bool axis_raw_data_like_set_axis_property(axis_msg_t *self, axis_list_t *paths,
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

  axis_list_foreach (paths, item_iter) {
    axis_value_path_item_t *item = axis_ptr_listnode_get(item_iter.node);
    axis_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if (!strcmp(axis_STR_NAME,
                    axis_string_get_raw_str(&item->obj_item_str))) {
          if (axis_value_is_string(value)) {
            axis_value_init_string_with_size(
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
