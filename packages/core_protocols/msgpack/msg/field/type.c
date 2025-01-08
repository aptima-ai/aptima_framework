//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "core_protocols/msgpack/msg/field/type.h"

#include <assert.h>

#include "core_protocols/msgpack/common/common.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "msgpack/pack.h"
#include "msgpack/unpack.h"
#include "axis_utils/macro/check.h"

void axis_msgpack_msg_type_serialize(axis_msg_t *self, msgpack_packer *pck) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self) && pck,
             "Invalid argument.");

  int rc = msgpack_pack_uint32(pck, self->type);
  axis_ASSERT(rc == 0, "Should not happen.");
}

axis_MSG_TYPE axis_msgpack_deserialize_msg_type(msgpack_unpacker *unpacker,
                                              msgpack_unpacked *unpacked) {
  axis_ASSERT(unpacker && unpacked, "Invalid argument.");

  axis_MSG_TYPE kind = axis_MSG_TYPE_INVALID;
  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    axis_ASSERT(MSGPACK_DATA_TYPE == MSGPACK_OBJECT_POSITIVE_INTEGER,
               "Invalid argument.");
    kind = (axis_MSG_TYPE)MSGPACK_DATA_I64;
  } else if (rc == MSGPACK_UNPACK_CONTINUE) {
    // msgpack format data is incomplete. Need to provide additional bytes.
    // Do nothing, return directly.
  } else {
    axis_ASSERT(0, "Should not happen.");
  }
  return kind;
}
