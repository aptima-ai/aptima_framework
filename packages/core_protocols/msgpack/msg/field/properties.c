//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//

#include "include_internal/ten_runtime/msg/field/properties.h"

#include <assert.h>

#include "core_protocols/msgpack/common/common.h"
#include "core_protocols/msgpack/common/value.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "msgpack/object.h"
#include "msgpack/pack.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/value_kv.h"

void ten_msgpack_msg_properties_serialize(ten_msg_t *self,
                                          msgpack_packer *pck) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && pck,
             "Invalid argument.");

  // Dump properties information if there is any.

  // Dump the size of properties[] first, so that the destination can
  // know the array size.
  int rc =
      msgpack_pack_uint32(pck, ten_list_size(ten_raw_msg_get_properties(self)));
  TEN_ASSERT(rc == 0, "Should not happen.");

  ten_list_foreach (ten_raw_msg_get_properties(self), iter) {
    ten_value_kv_t *property = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(property, "Invalid argument.");

    ten_msgpack_value_kv_serialize(property, pck);
  }
}

bool ten_msgpack_msg_properties_deserialize(ten_msg_t *self,
                                            msgpack_unpacker *unpacker,
                                            msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_POSITIVE_INTEGER) {
      // Save the value, otherwise, the afterwards unpacking would corrupt this
      // value.
      size_t properties_cnt = MSGPACK_DATA_I64;

      for (size_t i = 0; i < properties_cnt; i++) {
        ten_value_kv_t *kv =
            ten_msgpack_value_kv_deserialize(unpacker, unpacked);
        TEN_ASSERT(kv, "Should not happen.");

        ten_list_push_ptr_back(
            ten_raw_msg_get_properties(self), kv,
            (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);
      }

      return true;
    }
  }

  return false;
}
