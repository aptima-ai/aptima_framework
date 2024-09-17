//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "core_protocols/msgpack/msg/field/type.h"

#include <assert.h>

#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/macro/check.h"

void ten_msgpack_msg_type_serialize(ten_msg_t *self, msgpack_packer *pck) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && pck,
             "Invalid argument.");

  int rc = msgpack_pack_uint32(pck, self->type);
  TEN_ASSERT(rc == 0, "Should not happen.");
}
