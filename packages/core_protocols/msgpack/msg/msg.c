//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "include_internal/ten_runtime/msg/msg.h"

#include <string.h>

#include "core_protocols/msgpack/common/common.h"
#include "core_protocols/msgpack/common/parser.h"
#include "core_protocols/msgpack/msg/field/field_info.h"
#include "core_protocols/msgpack/msg/msg.h"
#include "core_protocols/msgpack/msg/msg_info.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/check.h"

void ten_msgpack_msghdr_serialize(ten_msg_t *self, msgpack_packer *pck) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && pck,
             "Invalid argument.");

  for (size_t i = 0; i < ten_protocol_msgpack_msg_fields_info_size; ++i) {
    ten_msg_field_serialize_func_t serialize =
        ten_protocol_msgpack_msg_fields_info[i].serialize;
    if (serialize) {
      serialize(self, pck);
    }
  }
}

bool ten_msgpack_msghdr_deserialize(ten_msg_t *self, msgpack_unpacker *unpacker,
                                    msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && ten_raw_msg_check_integrity(self) && unpacker && unpacked,
             "Invalid argument.");

  for (size_t i = 0; i < ten_protocol_msgpack_msg_fields_info_size; ++i) {
    ten_msg_field_deserialize_func_t deserialize =
        ten_protocol_msgpack_msg_fields_info[i].deserialize;
    if (deserialize) {
      if (!deserialize(self, unpacker, unpacked)) {
        return false;
      }
    }
  }

  return true;
}

bool ten_msgpack_msg_serialize(ten_shared_ptr_t *self, msgpack_packer *pck,
                               ten_error_t *err) {
  TEN_ASSERT(self && ten_msg_check_integrity(self) && pck, "Invalid argument.");

  ten_msg_serialize_func_t serialize =
      ten_msg_info[ten_msg_get_type(self)].serialize;
  if (serialize) {
    if (!serialize(self, pck, err)) {
      return false;
    }
  }

  return true;
}

bool ten_msgpack_msg_deserialize(ten_shared_ptr_t *self,
                                 msgpack_unpacker *unpacker,
                                 msgpack_unpacked *unpacked) {
  TEN_ASSERT(self && ten_msg_check_integrity(self) && unpacker && unpacked,
             "Invalid argument.");

  ten_msg_deserialize_func_t deserialize =
      ten_msg_info[ten_msg_get_type(self)].deserialize;
  if (deserialize) {
    return deserialize(self, unpacker, unpacked);
  }
  return false;
}

TEN_MSG_TYPE ten_msgpack_deserialize_msg_type(msgpack_unpacker *unpacker,
                                              msgpack_unpacked *unpacked) {
  TEN_ASSERT(unpacker && unpacked, "Invalid argument.");

  TEN_MSG_TYPE kind = TEN_MSG_TYPE_INVALID;
  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    TEN_ASSERT(MSGPACK_DATA_TYPE == MSGPACK_OBJECT_POSITIVE_INTEGER,
               "Invalid argument.");
    kind = (TEN_MSG_TYPE)MSGPACK_DATA_I64;
  } else if (rc == MSGPACK_UNPACK_CONTINUE) {
    // msgpack format data is incomplete. Need to provide additional bytes.
    // Do nothing, return directly.
  } else {
    TEN_ASSERT(0, "Should not happen.");
  }
  return kind;
}

ten_shared_ptr_t *ten_msgpack_deserialize_msg_internal(
    msgpack_unpacker *unpacker, msgpack_unpacked *unpacked) {
  TEN_ASSERT(unpacker && unpacked, "Invalid argument.");

  TEN_MSG_TYPE msg_type = ten_msgpack_deserialize_msg_type(unpacker, unpacked);
  ten_shared_ptr_t *new_msg = ten_msg_create_from_msg_type(msg_type);

  ten_msg_deserialize_func_t deserialize_func =
      ten_msg_info[msg_type].deserialize;
  if (deserialize_func) {
    if (deserialize_func(new_msg, unpacker, unpacked)) {
      return new_msg;
    } else {
      ten_shared_ptr_destroy(new_msg);
      return NULL;
    }
  } else {
    TEN_ASSERT(0, "Should handle more deserializable types.");
    return NULL;
  }
}

// Msgpack pack/unpack data in a unit of msgpack_object. In network
// transmissions, it's possible that a msgpack_object can _not_ be transmitted
// completed at once, or multiple msgpack_objects will be transmitted at once.
// There is no special relationships among multiple msgpack_objects in msgpack
// spec. If you pack and transmit a simple primitive type, such as int32, a
// simple corresponding msgpack unpack API could be used to unpack it in the
// other side. But if you want to pack and transmit a 'struct', a mechanism to
// enable the receiving end (that is, an unpacker) to know how to re-construct
// such 'struct' will be needed. There is one things to do to accomplish
// this:
//
// 1. The receiving end needs to know if all the data of a 'struct' is
//    received. Otherwise, it needs to know which parts of the 'struct' are
//    unpacked, and will resume the unpacking process from that point.
//
// If there is a mechanism to enable the receiving end to know if all the data
// of a 'struct' is received. Unpacking will be simpler.
//
// In order to accomplish this, there are several methods.
//
// 1. Transmit a 'size' and a 'type' before transmitting a 'struct', so that
//    the receiving end could know when all the data is received, and which
//    'struct' should be unpacked.
// 2. Wrap the 'struct' into a msgpack_array or msgpack_object, but this
//    method is suitable for simple 'struct', if the struct is complex, it's
//    hard to know the element count in advance before we are calling
//    msgpack_pack_array() or msgpack_pack_object().
// 3. Use msgpack_ext mechanism, that is to say, generate the packed data of
//    the 'struct' first, and then use msgpack_ext to transmit the generated
//    binary data to the receiving end. msgpack_ext is used to transmit a
//    binary data, and with a 'size' and a 'type'. This method is very much like
//    the method 1, sending 'size' and 'type' before sending the actual data,
//    but using msgpack mechanism to accomplish this.
//
// We are using method 3.
ten_buf_t ten_msgpack_serialize_msg(ten_list_t *msgs, ten_error_t *err) {
  bool result = true;

  msgpack_sbuffer sbuf;
  msgpack_sbuffer_init(&sbuf);

  msgpack_packer pck;
  msgpack_packer_init(&pck, &sbuf, msgpack_sbuffer_write);

  ten_list_foreach (msgs, iter) {
    msgpack_sbuffer sbuf_for_each_msg;
    msgpack_sbuffer_init(&sbuf_for_each_msg);

    msgpack_packer pck_for_each_msg;
    msgpack_packer_init(&pck_for_each_msg, &sbuf_for_each_msg,
                        msgpack_sbuffer_write);

    ten_shared_ptr_t *msg = ten_smart_ptr_listnode_get(iter.node);
    if (!ten_msgpack_msg_serialize(msg, &pck_for_each_msg, err)) {
      result = false;
    }

    ten_listnode_destroy(iter.node);

    if (result) {
      int rc = msgpack_pack_ext_with_body(&pck, sbuf_for_each_msg.data,
                                          sbuf_for_each_msg.size,
                                          TEN_MSGPACK_EXT_TYPE_MSG);
      TEN_ASSERT(rc == 0, "Should not happen.");
    }

    msgpack_sbuffer_destroy(&sbuf_for_each_msg);

    if (!result) {
      break;
    }
  }

  if (result) {
    // Note: The data in 'sbuf' would be freed later. However, it's kind of a
    // hack, because we are expecting the freeing method in TEN would be paired
    // with the allocating method in msgpack (ex: malloc, new). And it would
    // cause maintainability issues, ex: if the freeing method is TEN_FREE, it
    // would cause the internal errors in the TEN_MALLOC/TEN_FREE mechanism. The
    // correct way might be use TEN_MALLOC here to allocate a new memory buffer,
    // perform memcpy, but that would suffer some performance overhead. So think
    // twice if we really need to do this before using it.
    return TEN_BUF_STATIC_INIT_WITH_DATA_UNOWNED((uint8_t *)sbuf.data,
                                                 sbuf.size);
  } else {
    return TEN_BUF_STATIC_INIT_WITH_DATA_UNOWNED(NULL, 0);
  }
}

void ten_msgpack_deserialize_msg(ten_msgpack_parser_t *parser,
                                 ten_buf_t input_buf, ten_list_t *result_msgs) {
  TEN_ASSERT(parser, "Invalid argument.");

  // The 1st step is to use the parser for msgpack_ext object to ensure that we
  // have received all the data where step 2 is needed.
  ten_msgpack_parser_feed_data(parser, (const char *)input_buf.data,
                               input_buf.content_size);

  // The OS might gather multiple network packets and send to us at once, so
  // we need to parse all the received data, otherwise, we might not get a
  // next chance to be notified that we have remaining data to be processed.
  while (true) {
    ten_shared_ptr_t *msg = ten_msgpack_parser_parse_data(parser);
    if (!msg) {
      break;
    }

    msg = ten_msgpack_cmd_deserialize_through_json(msg);

    ten_list_push_smart_ptr_back(result_msgs, msg);
    ten_shared_ptr_destroy(msg);
  }
}
