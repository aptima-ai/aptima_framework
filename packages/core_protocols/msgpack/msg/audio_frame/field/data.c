//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "core_protocols/msgpack/msg/audio_frame/field/data.h"

#include <assert.h>

#include "core_protocols/msgpack/common/value.h"
#include "include_internal/ten_runtime/msg/audio_frame/audio_frame.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "msgpack/object.h"
#include "ten_runtime/msg/audio_frame/audio_frame.h"
#include "ten_utils/lib/alloc.h"

void ten_msgpack_audio_frame_data_serialize(ten_msg_t *self,
                                            msgpack_packer *pck) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && pck,
             "Invalid argument.");

  ten_audio_frame_t *audio_frame = (ten_audio_frame_t *)self;
  ten_msgpack_value_serialize(&audio_frame->data, pck);
}

bool ten_msgpack_audio_frame_data_deserialize(ten_msg_t *self,
                                              msgpack_unpacker *unpacker,
                                              msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  return ten_msgpack_value_deserialize_inplace(
      &((ten_audio_frame_t *)self)->data, unpacker, unpacked);
}
