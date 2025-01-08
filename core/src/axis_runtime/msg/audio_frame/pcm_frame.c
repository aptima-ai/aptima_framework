//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/audio_frame/audio_frame.h"
#include "include_internal/axis_runtime/msg/audio_frame/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_utils/value/value_path.h"
#include "include_internal/axis_utils/value/value_set.h"
#include "axis_runtime/msg/audio_frame/audio_frame.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"

bool axis_raw_audio_frame_check_integrity(axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_AUDIO_FRAME_SIGNATURE) {
    return false;
  }

  if (self->msg_hdr.type != axis_MSG_TYPE_AUDIO_FRAME) {
    return false;
  }

  return true;
}

int32_t axis_raw_audio_frame_get_samples_per_channel(axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_int32(&self->samples_per_channel, NULL);
}

axis_buf_t *axis_raw_audio_frame_peek_buf(axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_peek_buf(&self->buf);
}

int32_t axis_raw_audio_frame_get_sample_rate(axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_int32(&self->sample_rate, NULL);
}

uint64_t axis_raw_audio_frame_get_channel_layout(axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_uint64(&self->channel_layout, NULL);
}

bool axis_raw_audio_frame_is_eof(axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_bool(&self->is_eof, NULL);
}

int32_t axis_raw_audio_frame_get_line_size(axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_int32(&self->line_size, NULL);
}

int32_t axis_raw_audio_frame_get_bytes_per_sample(axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_int32(&self->bytes_per_sample, NULL);
}

int32_t axis_raw_audio_frame_get_number_of_channel(axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_int32(&self->number_of_channel, NULL);
}

axis_AUDIO_FRAME_DATA_FMT axis_raw_audio_frame_get_data_fmt(
    axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_int32(&self->data_fmt, NULL);
}

int64_t axis_raw_audio_frame_get_timestamp(axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_get_int64(&self->timestamp, NULL);
}

bool axis_raw_audio_frame_set_samples_per_channel(axis_audio_frame_t *self,
                                                 int32_t samples_per_channel) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_int32(&self->samples_per_channel, samples_per_channel);
}

bool axis_raw_audio_frame_set_sample_rate(axis_audio_frame_t *self,
                                         int32_t sample_rate) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_int32(&self->sample_rate, sample_rate);
}

bool axis_raw_audio_frame_set_channel_layout(axis_audio_frame_t *self,
                                            uint64_t channel_layout) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_uint64(&self->channel_layout, channel_layout);
}

bool axis_raw_audio_frame_set_eof(axis_audio_frame_t *self, bool is_eof) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_bool(&self->is_eof, is_eof);
}

bool axis_raw_audio_frame_set_line_size(axis_audio_frame_t *self,
                                       int32_t line_size) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_int32(&self->line_size, line_size);
}

bool axis_raw_audio_frame_set_bytes_per_sample(axis_audio_frame_t *self,
                                              int32_t bytes_per_sample) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_int32(&self->bytes_per_sample, bytes_per_sample);
}

bool axis_raw_audio_frame_set_number_of_channel(axis_audio_frame_t *self,
                                               int32_t number) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_int32(&self->number_of_channel, number);
}

bool axis_raw_audio_frame_set_data_fmt(axis_audio_frame_t *self,
                                      axis_AUDIO_FRAME_DATA_FMT data_fmt) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_int32(&self->data_fmt, data_fmt);
}

bool axis_raw_audio_frame_set_timestamp(axis_audio_frame_t *self,
                                       int64_t timestamp) {
  axis_ASSERT(self, "Should not happen.");
  return axis_value_set_int64(&self->timestamp, timestamp);
}

static uint8_t *axis_raw_audio_frame_alloc_buf(axis_audio_frame_t *self,
                                              size_t size) {
  if (size <= 0) {
    axis_ASSERT(0, "Should not happen.");
    return NULL;
  }

  axis_value_init_buf(&self->buf, size);

  return axis_value_peek_buf(&self->buf)->data;
}

static void axis_raw_audio_frame_init(axis_audio_frame_t *self) {
  axis_raw_msg_init(&self->msg_hdr, axis_MSG_TYPE_AUDIO_FRAME);
  axis_signature_set(&self->signature, axis_AUDIO_FRAME_SIGNATURE);

  axis_value_init_int64(&self->timestamp, 0);
  axis_value_init_uint64(&self->channel_layout, 0);

  axis_value_init_int32(&self->sample_rate, 0);
  axis_value_init_int32(&self->bytes_per_sample, 0);
  axis_value_init_int32(&self->samples_per_channel, 0);
  axis_value_init_int32(&self->number_of_channel, 0);
  axis_value_init_int32(&self->data_fmt, axis_AUDIO_FRAME_DATA_FMT_INTERLEAVE);
  axis_value_init_int32(&self->line_size, 0);
  axis_value_init_buf(&self->buf, 0);
  axis_value_init_bool(&self->is_eof, false);
}

static axis_audio_frame_t *axis_raw_audio_frame_create_empty(void) {
  axis_audio_frame_t *self = axis_MALLOC(sizeof(axis_audio_frame_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_raw_audio_frame_init(self);

  return self;
}

static axis_audio_frame_t *axis_raw_audio_frame_create(const char *name,
                                                     axis_error_t *err) {
  axis_audio_frame_t *self = axis_raw_audio_frame_create_empty();
  axis_raw_msg_set_name((axis_msg_t *)self, name, err);

  return self;
}

static axis_audio_frame_t *axis_raw_audio_frame_create_with_len(
    const char *name, size_t name_len, axis_error_t *err) {
  axis_audio_frame_t *self = axis_raw_audio_frame_create_empty();
  axis_raw_msg_set_name_with_len((axis_msg_t *)self, name, name_len, err);

  return self;
}

axis_shared_ptr_t *axis_audio_frame_create_empty(void) {
  return axis_shared_ptr_create(axis_raw_audio_frame_create_empty(),
                               axis_raw_audio_frame_destroy);
}

axis_shared_ptr_t *axis_audio_frame_create(const char *name, axis_error_t *err) {
  return axis_shared_ptr_create(axis_raw_audio_frame_create(name, err),
                               axis_raw_audio_frame_destroy);
}

axis_shared_ptr_t *axis_audio_frame_create_with_name_len(const char *name,
                                                       size_t name_len,
                                                       axis_error_t *err) {
  return axis_shared_ptr_create(
      axis_raw_audio_frame_create_with_len(name, name_len, err),
      axis_raw_audio_frame_destroy);
}

void axis_raw_audio_frame_destroy(axis_audio_frame_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_raw_msg_deinit(&self->msg_hdr);
  axis_value_deinit(&self->buf);

  axis_FREE(self);
}

int64_t axis_audio_frame_get_timestamp(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_get_timestamp(axis_shared_ptr_get_data(self));
}

bool axis_audio_frame_set_timestamp(axis_shared_ptr_t *self, int64_t timestamp) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_set_timestamp(axis_shared_ptr_get_data(self),
                                           timestamp);
}

int32_t axis_audio_frame_get_sample_rate(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_get_sample_rate(axis_shared_ptr_get_data(self));
}

bool axis_audio_frame_set_sample_rate(axis_shared_ptr_t *self,
                                     int32_t sample_rate) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_set_sample_rate(axis_shared_ptr_get_data(self),
                                             sample_rate);
}

uint64_t axis_audio_frame_get_channel_layout(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_get_channel_layout(axis_shared_ptr_get_data(self));
}

bool axis_audio_frame_set_channel_layout(axis_shared_ptr_t *self,
                                        uint64_t channel_layout) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_set_channel_layout(axis_shared_ptr_get_data(self),
                                                channel_layout);
}

bool axis_audio_frame_is_eof(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_is_eof(axis_shared_ptr_get_data(self));
}

bool axis_audio_frame_set_eof(axis_shared_ptr_t *self, bool is_eof) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_set_eof(axis_shared_ptr_get_data(self), is_eof);
}

int32_t axis_audio_frame_get_samples_per_channel(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_get_samples_per_channel(
      axis_shared_ptr_get_data(self));
}

bool axis_audio_frame_set_samples_per_channel(axis_shared_ptr_t *self,
                                             int32_t samples_per_channel) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_set_samples_per_channel(
      axis_shared_ptr_get_data(self), samples_per_channel);
}

axis_buf_t *axis_audio_frame_peek_buf(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_peek_buf(axis_shared_ptr_get_data(self));
}

int32_t axis_audio_frame_get_line_size(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_get_line_size(axis_shared_ptr_get_data(self));
}

bool axis_audio_frame_set_line_size(axis_shared_ptr_t *self, int32_t line_size) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_set_line_size(axis_shared_ptr_get_data(self),
                                           line_size);
}

int32_t axis_audio_frame_get_bytes_per_sample(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_get_bytes_per_sample(
      axis_shared_ptr_get_data(self));
}

bool axis_audio_frame_set_bytes_per_sample(axis_shared_ptr_t *self,
                                          int32_t size) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_set_bytes_per_sample(axis_shared_ptr_get_data(self),
                                                  size);
}

int32_t axis_audio_frame_get_number_of_channel(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_get_number_of_channel(
      axis_shared_ptr_get_data(self));
}

bool axis_audio_frame_set_number_of_channel(axis_shared_ptr_t *self,
                                           int32_t number) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_set_number_of_channel(
      axis_shared_ptr_get_data(self), number);
}

axis_AUDIO_FRAME_DATA_FMT axis_audio_frame_get_data_fmt(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_get_data_fmt(axis_shared_ptr_get_data(self));
}

bool axis_audio_frame_set_data_fmt(axis_shared_ptr_t *self,
                                  axis_AUDIO_FRAME_DATA_FMT data_fmt) {
  axis_ASSERT(self, "Should not happen.");
  return axis_raw_audio_frame_set_data_fmt(axis_shared_ptr_get_data(self),
                                          data_fmt);
}

uint8_t *axis_audio_frame_alloc_buf(axis_shared_ptr_t *self, size_t size) {
  axis_ASSERT(self, "Should not happen.");

  return axis_raw_audio_frame_alloc_buf(axis_shared_ptr_get_data(self), size);
}

axis_msg_t *axis_raw_audio_frame_as_msg_clone(axis_msg_t *self,
                                            axis_list_t *excluded_field_ids) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self) &&
                 axis_raw_msg_get_type(self) == axis_MSG_TYPE_AUDIO_FRAME,
             "Should not happen.");

  axis_audio_frame_t *new_frame =
      (axis_audio_frame_t *)axis_MALLOC(sizeof(axis_audio_frame_t));
  axis_ASSERT(new_frame, "Failed to allocate memory.");

  axis_raw_audio_frame_init(new_frame);

  axis_audio_frame_t *self_frame = (axis_audio_frame_t *)self;

  axis_value_copy(&self_frame->timestamp, &new_frame->timestamp);
  axis_value_copy(&self_frame->sample_rate, &new_frame->sample_rate);
  axis_value_copy(&self_frame->bytes_per_sample, &new_frame->bytes_per_sample);
  axis_value_copy(&self_frame->samples_per_channel,
                 &new_frame->samples_per_channel);
  axis_value_copy(&self_frame->number_of_channel, &new_frame->number_of_channel);
  axis_value_copy(&self_frame->channel_layout, &new_frame->channel_layout);
  axis_value_copy(&self_frame->data_fmt, &new_frame->data_fmt);
  axis_value_copy(&self_frame->line_size, &new_frame->line_size);
  axis_value_copy(&self_frame->is_eof, &new_frame->is_eof);
  axis_value_copy(&self_frame->buf, &new_frame->buf);

  for (size_t i = 0; i < axis_audio_frame_fields_info_size; ++i) {
    if (excluded_field_ids) {
      bool skip = false;

      axis_list_foreach (excluded_field_ids, iter) {
        if (axis_audio_frame_fields_info[i].field_id ==
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
        axis_audio_frame_fields_info[i].copy_field;
    if (copy_field) {
      copy_field((axis_msg_t *)new_frame, self, excluded_field_ids);
    }
  }

  return (axis_msg_t *)new_frame;
}

axis_value_t *axis_raw_audio_frame_peek_axis_property(axis_msg_t *self,
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

  axis_audio_frame_t *audio_frame = (axis_audio_frame_t *)self;

  axis_list_foreach (paths, item_iter) {
    axis_value_path_item_t *item = axis_ptr_listnode_get(item_iter.node);
    axis_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if (!strcmp(axis_STR_BYTES_PER_SAMPLE,
                    axis_string_get_raw_str(&item->obj_item_str))) {
          result = &audio_frame->bytes_per_sample;
        } else if (!strcmp(axis_STR_TIMESTAMP,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &audio_frame->timestamp;
        } else if (!strcmp(axis_STR_CHANNEL_LAYOUT,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &audio_frame->channel_layout;
        } else if (!strcmp(axis_STR_DATA_FMT,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &audio_frame->data_fmt;
        } else if (!strcmp(axis_STR_IS_EOF,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &audio_frame->is_eof;
        } else if (!strcmp(axis_STR_LINE_SIZE,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &audio_frame->line_size;
        } else if (!strcmp(axis_STR_NUMBER_OF_CHANNEL,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &audio_frame->number_of_channel;
        } else if (!strcmp(axis_STR_SAMPLE_RATE,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &audio_frame->sample_rate;
        } else if (!strcmp(axis_STR_SAMPLES_PER_CHANNEL,
                           axis_string_get_raw_str(&item->obj_item_str))) {
          result = &audio_frame->samples_per_channel;
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

bool axis_raw_audio_frame_loop_all_fields(
    axis_msg_t *self, axis_raw_msg_process_one_field_func_t cb, void *user_data,
    axis_error_t *err) {
  axis_ASSERT(
      self && axis_raw_audio_frame_check_integrity((axis_audio_frame_t *)self),
      "Invalid argument.");

  for (size_t i = 0; i < axis_audio_frame_fields_info_size; ++i) {
    axis_msg_process_field_func_t process_field =
        axis_audio_frame_fields_info[i].process_field;
    if (process_field) {
      if (!process_field(self, cb, user_data, err)) {
        return false;
      }
    }
  }

  return true;
}
