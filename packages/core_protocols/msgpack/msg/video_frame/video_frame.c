//
// This file is part of the TEN Framework project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "core_protocols/msgpack/msg/video_frame/video_frame.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "core_protocols/msgpack/msg/video_frame/field/field_info.h"
#include "include_internal/ten_runtime/msg/video_frame/video_frame.h"
#include "ten_runtime/msg/video_frame/video_frame.h"

bool ten_msgpack_video_frame_serialize(ten_shared_ptr_t *self_,
                                       msgpack_packer *pck, ten_error_t *err) {
  TEN_ASSERT(self_ && pck, "Invalid argument.");

  ten_video_frame_t *self = ten_shared_ptr_get_data(self_);

  for (size_t i = 0; i < ten_video_frame_fields_info_size; ++i) {
    ten_msg_field_serialize_func_t serialize =
        ten_video_frame_fields_info[i].serialize;
    if (serialize) {
      serialize(&self->msg_hdr, pck);
    }
  }

  return true;
}

bool ten_msgpack_video_frame_deserialize(ten_shared_ptr_t *self,
                                         msgpack_unpacker *unpacker,
                                         msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  ten_video_frame_t *raw_msg = ten_shared_ptr_get_data(self);

  for (size_t i = 0; i < ten_video_frame_fields_info_size; ++i) {
    ten_msg_field_deserialize_func_t deserialize =
        ten_video_frame_fields_info[i].deserialize;
    if (deserialize) {
      if (!deserialize(&raw_msg->msg_hdr, unpacker, unpacked)) {
        return false;
      }
    }
  }

  return true;
}
