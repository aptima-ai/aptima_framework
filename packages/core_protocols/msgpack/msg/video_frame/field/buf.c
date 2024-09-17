//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "core_protocols/msgpack/msg/video_frame/field/buf.h"

#include <assert.h>

#include "core_protocols/msgpack/common/value.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "include_internal/ten_runtime/msg/video_frame/video_frame.h"
#include "ten_runtime/msg/video_frame/video_frame.h"

void ten_msgpack_video_frame_buf_serialize(ten_msg_t *self,
                                           msgpack_packer *pck) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && pck,
             "Invalid argument.");

  ten_video_frame_t *video_frame = (ten_video_frame_t *)self;
  ten_msgpack_value_serialize(&video_frame->data, pck);
}

bool ten_msgpack_video_frame_buf_deserialize(ten_msg_t *self,
                                             msgpack_unpacker *unpacker,
                                             msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  return ten_msgpack_value_deserialize_inplace(
      &((ten_video_frame_t *)self)->data, unpacker, unpacked);
}
