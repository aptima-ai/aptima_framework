//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "core_protocols/msgpack/common/parser.h"

#include <stdlib.h>

#include "core_protocols/msgpack/common/common.h"
#include "core_protocols/msgpack/msg/msg.h"
#include "msgpack.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

void axis_msgpack_parser_init(axis_msgpack_parser_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_UNUSED bool result =
      msgpack_unpacker_init(&self->unpacker, MSGPACK_UNPACKER_INIT_BUFFER_SIZE);
  axis_ASSERT(result, "Should not happen.");

  msgpack_unpacked_init(&self->unpacked);
}

void axis_msgpack_parser_deinit(axis_msgpack_parser_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  msgpack_unpacker_destroy(&self->unpacker);
  msgpack_unpacked_destroy(&self->unpacked);
}

void axis_msgpack_parser_feed_data(axis_msgpack_parser_t *self, const char *data,
                                  size_t data_size) {
  axis_ASSERT(self, "Invalid argument.");

  // Make sure there's enough room, or expand the unpacker accordingly.
  if (msgpack_unpacker_buffer_capacity(&self->unpacker) < data_size) {
    axis_UNUSED bool result =
        msgpack_unpacker_reserve_buffer(&self->unpacker, data_size);

    axis_ASSERT(result && msgpack_unpacker_buffer_capacity(&self->unpacker) >=
                             data_size,
               "Failed to allocate memory.");
  }

  // Copy the data to the unpacker.
  memcpy(msgpack_unpacker_buffer(&self->unpacker), data, data_size);

  // Tell unpacker how much data is fed into.
  msgpack_unpacker_buffer_consumed(&self->unpacker, data_size);
}

axis_shared_ptr_t *axis_msgpack_parser_parse_data(axis_msgpack_parser_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_shared_ptr_t *new_msg = NULL;

  msgpack_unpack_return rc =
      msgpack_unpacker_next(&self->unpacker, &self->unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (self->unpacked.data.type != MSGPACK_OBJECT_EXT) {
#define UNPACKED_BUFFER_SIZE 2048
      char unpacked_buffer[2048];
      msgpack_object_print_buffer(unpacked_buffer, UNPACKED_BUFFER_SIZE,
                                  self->unpacked.data);
      axis_LOGE("%s", unpacked_buffer);

      axis_ASSERT(0, "Should receive a msgpack ext object, but receive type(%d)",
                 self->unpacked.data.type);
    }
    axis_ASSERT(self->unpacked.data.via.ext.type == axis_MSGPACK_EXT_TYPE_MSG,
               "The only supported msgpack ext object type is TEN "
               "axis_msg_t, but receive type(%d)",
               self->unpacked.data.via.ext.type);

    axis_msgpack_parser_t msg_parser;
    axis_msgpack_parser_init(&msg_parser);

    // Feed data gathered from step 1 (parsing the msgpack ext object) to the
    // step 2 (parsing the TEN object).
    axis_msgpack_parser_feed_data(&msg_parser, self->unpacked.data.via.ext.ptr,
                                 self->unpacked.data.via.ext.size);

    new_msg =
        axis_msgpack_deserialize_msg(&msg_parser.unpacker, &msg_parser.unpacked);

    axis_msgpack_parser_deinit(&msg_parser);
  } else if (rc == MSGPACK_UNPACK_CONTINUE) {
    // msgpack format data is incomplete. Need to provide additional bytes.
    // Do nothing, return directly.
  } else {
    axis_ASSERT(0, "Should not happen.");
  }

  return new_msg;
}
