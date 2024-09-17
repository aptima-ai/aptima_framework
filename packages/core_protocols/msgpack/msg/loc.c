//
// This file is part of the TEN Framework project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "core_protocols/msgpack/msg/loc.h"

#include <stdlib.h>

#include "core_protocols/msgpack/common/common.h"
#include "ten_utils/macro/check.h"
#include "msgpack/object.h"
#include "msgpack/pack.h"
#include "msgpack/unpack.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/lib/string.h"

void ten_msgpack_loc_serialize(ten_loc_t *self, msgpack_packer *pck) {
  TEN_ASSERT(self && pck, "Invalid argument.");

  int rc =
      msgpack_pack_str_with_body(pck, ten_string_get_raw_str(&self->app_uri),
                                 ten_string_len(&self->app_uri));
  TEN_ASSERT(rc == 0, "Should not happen.");

  rc =
      msgpack_pack_str_with_body(pck, ten_string_get_raw_str(&self->graph_name),
                                 ten_string_len(&self->graph_name));
  TEN_ASSERT(rc == 0, "Should not happen.");

  rc = msgpack_pack_str_with_body(
      pck, ten_string_get_raw_str(&self->extension_group_name),
      ten_string_len(&self->extension_group_name));
  TEN_ASSERT(rc == 0, "Should not happen.");

  rc = msgpack_pack_str_with_body(pck,
                                  ten_string_get_raw_str(&self->extension_name),
                                  ten_string_len(&self->extension_name));
  TEN_ASSERT(rc == 0, "Should not happen.");
}

void ten_msgpack_loc_deserialize(ten_loc_t *self, msgpack_unpacker *unpacker,
                                 msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && unpacker && unpacked, "Invalid argument.");

  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_STR) {
      ten_string_set_formatted(&self->app_uri, "%.*s", MSGPACK_DATA_STR_SIZE,
                               MSGPACK_DATA_STR_PTR);
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
  } else {
    TEN_ASSERT(0, "Should not happen.");
  }

  rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_STR) {
      ten_string_set_formatted(&self->graph_name, "%.*s", MSGPACK_DATA_STR_SIZE,
                               MSGPACK_DATA_STR_PTR);
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
  } else {
    TEN_ASSERT(0, "Should not happen.");
  }

  rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_STR) {
      ten_string_set_formatted(&self->extension_group_name, "%.*s",
                               MSGPACK_DATA_STR_SIZE, MSGPACK_DATA_STR_PTR);
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
  } else {
    TEN_ASSERT(0, "Should not happen.");
  }

  rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_STR) {
      ten_string_set_formatted(&self->extension_name, "%.*s",
                               MSGPACK_DATA_STR_SIZE, MSGPACK_DATA_STR_PTR);
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
  } else {
    TEN_ASSERT(0, "Should not happen.");
  }
}

void ten_msgpack_loc_list_serialize(ten_list_t *self, msgpack_packer *pck) {
  TEN_ASSERT(self && pck, "Invalid argument.");

  // Do _not_ use msgpack_pack_array() here, refer to the comments in
  // `common/value.c`.
  int rc = msgpack_pack_uint32(pck, ten_list_size(self));
  TEN_ASSERT(rc == 0, "Should not happen.");

  ten_list_foreach (self, iter) {
    ten_loc_t *loc = ten_ptr_listnode_get(iter.node);
    ten_msgpack_loc_serialize(loc, pck);
  }
}

ten_list_t ten_msgpack_loc_list_deserialize(msgpack_unpacker *unpacker,
                                            msgpack_unpacked *unpacked) {
  TEN_ASSERT(unpacker, "Invalid argument.");
  TEN_ASSERT(unpacked, "Invalid argument.");

  ten_list_t result = TEN_LIST_INIT_VAL;

  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_POSITIVE_INTEGER) {
      size_t loc_cnt = MSGPACK_DATA_I64;
      for (size_t i = 0; i < loc_cnt; i++) {
        ten_loc_t *loc = ten_loc_create(NULL, NULL, NULL, NULL, NULL);
        ten_msgpack_loc_deserialize(loc, unpacker, unpacked);
        ten_list_push_ptr_back(
            &result, loc, (ten_ptr_listnode_destroy_func_t)ten_loc_destroy);
      }
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
  } else {
    TEN_ASSERT(0, "Should not happen.");
  }

  return result;
}
