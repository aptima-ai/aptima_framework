//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/video_frame/video_frame.h"

#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg/video_frame/field/field_info.h"
#include "include_internal/axis_utils/value/value_path.h"
#include "include_internal/axis_utils/value/value_set.h"
#include "axis_runtime/msg/msg.h"
#include "axis_runtime/msg/video_frame/video_frame.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"

bool axis_raw_video_frame_check_integrity(axis_video_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_VIDEO_FRAME_SIGNATURE) {
    return false;
  }

  if (self->msg_hdr.type != axis_MSG_TYPE_VIDEO_FRAME) {
    return false;
  }

  return true;
}

// Raw video frame
int64_t axis_raw_video_frame_get_timestamp(axis_video_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_int64(&self->timestamp, NULL);
}

int32_t axis_raw_video_frame_get_width(axis_video_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_int32(&self->width, NULL);
}

int32_t axis_raw_video_frame_get_height(axis_video_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_int32(&self->height, NULL);
}

static axis_buf_t *axis_raw_video_frame_peek_buf(axis_video_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_peek_buf(&self->data);
}

axis_buf_t *axis_video_frame_peek_buf(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_peek_buf(axis_shared_ptr_get_data(self));
}

axis_PIXEL_FMT axis_raw_video_frame_get_pixel_fmt(axis_video_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_int32(&self->pixel_fmt, NULL);
}

bool axis_raw_video_frame_is_eof(axis_video_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_bool(&self->is_eof, NULL);
}

bool axis_raw_video_frame_set_width(axis_video_frame_t *self, int32_t width) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_int32(&self->width, width);
}

bool axis_raw_video_frame_set_height(axis_video_frame_t *self, int32_t height) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_int32(&self->height, height);
}

bool axis_raw_video_frame_set_timestamp(axis_video_frame_t *self,
                                       int64_t timestamp) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_int64(&self->timestamp, timestamp);
}

bool axis_raw_video_frame_set_pixel_fmt(axis_video_frame_t *self,
                                       axis_PIXEL_FMT pixel_fmt) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_int32(&self->pixel_fmt, pixel_fmt);
}

bool axis_raw_video_frame_set_eof(axis_video_frame_t *self, bool is_eof) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_bool(&self->is_eof, is_eof);
}

void axis_raw_video_frame_init(axis_video_frame_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_raw_msg_init(&self->msg_hdr, axis_MSG_TYPE_VIDEO_FRAME);
  axis_signature_set(&self->signature, axis_VIDEO_FRAME_SIGNATURE);

  axis_value_init_int32(&self->pixel_fmt, axis_PIXEL_FMT_RGBA);
  axis_value_init_int64(&self->timestamp, 0);
  axis_value_init_int32(&self->width, 0);
  axis_value_init_int32(&self->height, 0);
  axis_value_init_bool(&self->is_eof, false);
  axis_value_init_buf(&self->data, 0);
}

static axis_video_frame_t *axis_raw_video_frame_create_empty(void) {
  axis_video_frame_t *self = axis_MALLOC(sizeof(axis_video_frame_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_raw_video_frame_init(self);

  return self;
}

static axis_video_frame_t *axis_raw_video_frame_create(const char *name,
                                                     axis_error_t *err) {
  axis_ASSERT(name, "Invalid argument.");

  axis_video_frame_t *self = axis_raw_video_frame_create_empty();
  axis_raw_msg_set_name((axis_msg_t *)self, name, err);

  return self;
}

static axis_video_frame_t *axis_raw_video_frame_create_with_name_len(
    const char *name, size_t name_len, axis_error_t *err) {
  axis_ASSERT(name, "Invalid argument.");

  axis_video_frame_t *self = axis_raw_video_frame_create_empty();
  axis_raw_msg_set_name_with_len((axis_msg_t *)self, name, name_len, err);

  return self;
}

void axis_raw_video_frame_destroy(axis_video_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_msg_deinit(&self->msg_hdr);

  axis_value_deinit(&self->data);

  axis_FREE(self);
}

axis_shared_ptr_t *axis_video_frame_create_empty(void) {
  return axis_shared_ptr_create(axis_raw_video_frame_create_empty(),
                               axis_raw_video_frame_destroy);
}

axis_shared_ptr_t *axis_video_frame_create(const char *name, axis_error_t *err) {
  return axis_shared_ptr_create(axis_raw_video_frame_create(name, err),
                               axis_raw_video_frame_destroy);
}

axis_shared_ptr_t *axis_video_frame_create_with_name_len(const char *name,
                                                       size_t name_len,
                                                       axis_error_t *err) {
  return axis_shared_ptr_create(
      axis_raw_video_frame_create_with_name_len(name, name_len, err),
      axis_raw_video_frame_destroy);
}

int32_t axis_video_frame_get_width(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_get_width(axis_shared_ptr_get_data(self));
}

bool axis_video_frame_set_width(axis_shared_ptr_t *self, int32_t width) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_set_width(axis_shared_ptr_get_data(self), width);
}

int32_t axis_video_frame_get_height(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_get_height(axis_shared_ptr_get_data(self));
}

bool axis_video_frame_set_height(axis_shared_ptr_t *self, int32_t height) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_set_height(axis_shared_ptr_get_data(self), height);
}

static uint8_t *axis_raw_video_frame_alloc_data(axis_video_frame_t *self,
                                               size_t size) {
  uint8_t *data = axis_value_peek_buf(&self->data)->data;
  if (data) {
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  if (!axis_buf_init_with_owned_data(axis_value_peek_buf(&self->data), size)) {
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  return axis_value_peek_buf(&self->data)->data;
}

uint8_t *axis_video_frame_alloc_data(axis_shared_ptr_t *self, size_t size) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_alloc_data(axis_shared_ptr_get_data(self), size);
}

int64_t axis_video_frame_get_timestamp(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_get_timestamp(axis_shared_ptr_get_data(self));
}

bool axis_video_frame_set_timestamp(axis_shared_ptr_t *self, int64_t timestamp) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_set_timestamp(axis_shared_ptr_get_data(self),
                                           timestamp);
}

axis_PIXEL_FMT axis_video_frame_get_pixel_fmt(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_get_pixel_fmt(axis_shared_ptr_get_data(self));
}

bool axis_video_frame_set_pixel_fmt(axis_shared_ptr_t *self, axis_PIXEL_FMT type) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_set_pixel_fmt(axis_shared_ptr_get_data(self), type);
}

bool axis_video_frame_is_eof(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_is_eof(axis_shared_ptr_get_data(self));
}

bool axis_video_frame_set_eof(axis_shared_ptr_t *self, bool is_eof) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_video_frame_set_eof(axis_shared_ptr_get_data(self), is_eof);
}

axis_msg_t *axis_raw_video_frame_as_msg_clone(axis_msg_t *self,
                                            axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self) &&
                 axis_raw_msg_get_type(self) == axis_MSG_TYPE_VIDEO_FRAME,
             "Should not happen.");

  axis_video_frame_t *new_frame =
      (axis_video_frame_t *)axis_MALLOC(sizeof(axis_video_frame_t));
  axis_ASSERT(new_frame, "Failed to allocate memory.");

  axis_raw_video_frame_init(new_frame);

  axis_video_frame_t *src_frame = (axis_video_frame_t *)self;
  axis_value_copy(&src_frame->timestamp, &new_frame->timestamp);
  axis_value_copy(&src_frame->width, &new_frame->width);
  axis_value_copy(&src_frame->height, &new_frame->height);
  axis_value_copy(&src_frame->is_eof, &new_frame->is_eof);
  axis_value_copy(&src_frame->pixel_fmt, &new_frame->pixel_fmt);
  axis_value_copy(&src_frame->data, &new_frame->data);

  for (size_t i = 0; i < axis_video_frame_fields_info_size; ++i) {
    if (excluded_field_ids) {
      bool skip = false;

      axis_list_foreach (excluded_field_ids, iter) {
        if (axis_video_frame_fields_info[i].field_id ==
            axis_int32_listnode_get(iter.node)) {
          skip = true;
          break;
        }
      }

      if (skip) {
        continue;
      }
    }

    axis_msg_copy_field_func_t copy_field =
        axis_video_frame_fields_info[i].copy_field;
    if (copy_field) {
      copy_field((axis_msg_t *)new_frame, self, excluded_field_ids);
    }
  }

  return (axis_msg_t *)new_frame;
}

bool axis_raw_video_frame_set_axis_property(axis_msg_t *self, axis_list_t *paths,
                                          axis_value_t *value,
                                          axis_error_t *err) {
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

  axis_video_frame_t *video_frame = (axis_video_frame_t *)self;

  axis_list_foreach (paths, item_iter) {
    axis_value_path_item_t *item = axis_ptr_listnode_get(item_iter.node);
    axis_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if (!strcmp(axis_STR_PIXEL_FMT,
                    axis_string_get_raw_str(&item->obj_item_str))) {
          const char *pixel_fmt_str = axis_value_peek_raw_str(value, err);
          axis_raw_video_frame_set_pixel_fmt(
              video_frame,
              axis_video_frame_pixel_fmt_from_string(pixel_fmt_str));
          success = axis_error_is_success(err);
        } else if (!strcmp(axis_STR_TIMESTAMP,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          axis_raw_video_frame_set_timestamp(video_frame,
                                            axis_value_get_int64(value, err));
          success = axis_error_is_success(err);
        } else if (!strcmp(axis_STR_WIDTH,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          axis_raw_video_frame_set_width(video_frame,
                                        axis_value_get_int32(value, err));
          success = axis_error_is_success(err);
        } else if (!strcmp(axis_STR_HEIGHT,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          axis_raw_video_frame_set_height(video_frame,
                                         axis_value_get_int32(value, err));
          success = axis_error_is_success(err);
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

axis_value_t *axis_raw_video_frame_peek_axis_property(axis_msg_t *self,
                                                   axis_list_t *paths,
                                                   axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Invalid argument.");
  axis_ASSERT(paths && axis_list_check_integrity(paths),
             "path should not be empty.");

  axis_value_t *result = NULL;

  axis_error_t tmp_err;
  bool use_tmp_err = false;
  if (!err) {
    use_tmp_err = true;
    axis_error_init(&tmp_err);
    err = &tmp_err;
  }

  axis_video_frame_t *video_frame = (axis_video_frame_t *)self;

  axis_list_foreach (paths, item_iter) {
    axis_value_path_item_t *item = axis_ptr_listnode_get(item_iter.node);
    axis_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if (!strcmp(axis_STR_PIXEL_FMT,
                    axis_string_get_raw_str(&item->obj_item_str))) {
          result = &video_frame->pixel_fmt;
        } else if (!strcmp(axis_STR_TIMESTAMP,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &video_frame->timestamp;
        } else if (!strcmp(axis_STR_WIDTH,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &video_frame->width;
        } else if (!strcmp(axis_STR_HEIGHT,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &video_frame->height;
        } else if (!strcmp(axis_STR_IS_EOF,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &video_frame->is_eof;
        } else if (!strcmp(axis_STR_DATA,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &video_frame->data;
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

  return result;
}

bool axis_raw_video_frame_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(
      self && axis_raw_video_frame_check_integrity((axis_video_frame_t *)self),
      "Should not happen.");

  for (size_t i = 0; i < axis_video_frame_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_video_frame_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}
