//
// This file is part of the TEN Framework project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include <assert.h>

#include "core_protocols/msgpack/common/common.h"
#include "core_protocols/msgpack/msg/field/dest.h"
#include "core_protocols/msgpack/msg/loc.h"
#include "core_protocols/msgpack/msg/msg.h"
#include "ten_utils/macro/check.h"

void ten_msgpack_msg_src_serialize(ten_msg_t *self, msgpack_packer *pck) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && pck,
             "Invalid argument.");

  ten_msgpack_loc_serialize(&self->src_loc, pck);
}

bool ten_msgpack_msg_src_deserialize(ten_msg_t *self,
                                     msgpack_unpacker *unpacker,
                                     msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  ten_msgpack_loc_deserialize(&self->src_loc, unpacker, unpacked);
  return true;
}
