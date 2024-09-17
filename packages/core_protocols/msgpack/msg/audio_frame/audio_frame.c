//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "ten_runtime/msg/audio_frame/audio_frame.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "core_protocols/msgpack/msg/audio_frame/audio_frame.h"
#include "core_protocols/msgpack/msg/audio_frame/field/field_info.h"
#include "include_internal/ten_runtime/msg/audio_frame/audio_frame.h"

bool ten_msgpack_audio_frame_serialize(ten_shared_ptr_t *self_,
                                       msgpack_packer *pck, ten_error_t *err) {
  TEN_ASSERT(self_ && pck, "Invalid argument.");

  ten_audio_frame_t *self = ten_shared_ptr_get_data(self_);

  for (size_t i = 0; i < ten_audio_frame_fields_info_size; ++i) {
    ten_msg_field_serialize_func_t serialize =
        ten_audio_frame_fields_info[i].serialize;
    if (serialize) {
      serialize(&self->msg_hdr, pck);
    }
  }

  return true;
}

bool ten_msgpack_audio_frame_deserialize(ten_shared_ptr_t *self,
                                         msgpack_unpacker *unpacker,
                                         msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  ten_audio_frame_t *raw_msg = ten_shared_ptr_get_data(self);

  for (size_t i = 0; i < ten_audio_frame_fields_info_size; ++i) {
    ten_msg_field_deserialize_func_t deserialize =
        ten_audio_frame_fields_info[i].deserialize;
    if (deserialize) {
      if (!deserialize(&raw_msg->msg_hdr, unpacker, unpacked)) {
        return false;
      }
    }
  }

  return true;
}
