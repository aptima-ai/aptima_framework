//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include "aptima_utils/value/value.h"

#include "core_protocols/msgpack/common/common.h"
#include "core_protocols/msgpack/common/value.h"
#include "include_internal/aptima_utils/value/value_kv.h"
#include "include_internal/aptima_utils/value/value_set.h"
#include "msgpack/object.h"
#include "msgpack/pack.h"
#include "msgpack/unpack.h"
#include "aptima_utils/container/list.h"
#include "aptima_utils/container/list_ptr.h"
#include "aptima_utils/lib/buf.h"
#include "aptima_utils/lib/error.h"
#include "aptima_utils/macro/check.h"
#include "aptima_utils/value/type.h"
#include "aptima_utils/value/value_get.h"
#include "aptima_utils/value/value_kv.h"

bool aptima_msgpack_value_deserialize(aptima_value_t *value,
                                   msgpack_unpacker *unpacker,
                                   msgpack_unpacked *unpacked) {
  aptima_ASSERT(unpacker, "Invalid argument.");
  aptima_ASSERT(unpacked, "Invalid argument.");

  bool result = true;

  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_POSITIVE_INTEGER) {
      switch (MSGPACK_DATA_I64) {
        case aptima_TYPE_INT8: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_set_int8(value, MSGPACK_DATA_I64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_INT16: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_set_int16(value, MSGPACK_DATA_I64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_INT32: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_set_int32(value, MSGPACK_DATA_I64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_INT64: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_set_int64(value, MSGPACK_DATA_I64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_UINT8: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_set_uint8(value, MSGPACK_DATA_U64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_UINT16: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_set_uint16(value, MSGPACK_DATA_U64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_UINT32: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_set_uint32(value, MSGPACK_DATA_U64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_UINT64: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_set_uint64(value, MSGPACK_DATA_U64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_FLOAT32: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_set_float32(value, MSGPACK_DATA_F64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_FLOAT64: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_set_float64(value, MSGPACK_DATA_F64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_BOOL: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_set_bool(value, MSGPACK_DATA_BOOL);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_STRING: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_STR) {
              aptima_value_set_string_with_size(value, MSGPACK_DATA_STR_PTR,
                                             MSGPACK_DATA_STR_SIZE);
              goto done;
            } else {
              aptima_ASSERT(0, "Should not happen.");
            }
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_BUF: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_BIN) {
              aptima_buf_t *buf = aptima_value_peek_buf(value);
              aptima_ASSERT(buf && aptima_buf_check_integrity(buf),
                         "Invalid argument.");

              // To overwrite the old buffer, we deinit it first.
              aptima_buf_deinit(buf);
              aptima_buf_init_with_copying_data(
                  buf, (uint8_t *)MSGPACK_DATA_BIN_PTR, MSGPACK_DATA_BIN_SIZE);
              goto done;
            } else {
              aptima_ASSERT(0, "Should not happen.");
            }
          }
          break;
        }

        case aptima_TYPE_ARRAY: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_POSITIVE_INTEGER) {
              aptima_list_t array = aptima_LIST_INIT_VAL;

              size_t array_items_cnt = MSGPACK_DATA_I64;
              for (size_t i = 0; i < array_items_cnt; i++) {
                aptima_value_t *item =
                    aptima_msgpack_create_value_through_deserialization(unpacker,
                                                                     unpacked);
                if (!item) {
                  goto error;
                }
                aptima_list_push_ptr_back(
                    &array, item,
                    (aptima_ptr_listnode_destroy_func_t)aptima_value_destroy);
              }

              aptima_value_set_array_with_move(value, &array);
              goto done;
            } else {
              aptima_ASSERT(0, "Should not happen.");
            }
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_OBJECT: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_POSITIVE_INTEGER) {
              aptima_list_t kv_list = aptima_LIST_INIT_VAL;

              size_t object_kv_cnt = MSGPACK_DATA_I64;
              for (size_t i = 0; i < object_kv_cnt; i++) {
                aptima_value_kv_t *kv =
                    aptima_msgpack_create_value_kv_through_deserialization(
                        unpacker, unpacked);
                if (!kv) {
                  goto error;
                }
                aptima_list_push_ptr_back(
                    &kv_list, kv,
                    (aptima_ptr_listnode_destroy_func_t)aptima_value_kv_destroy);
              }

              aptima_value_set_object_with_move(value, &kv_list);
              goto done;
            } else {
              aptima_ASSERT(0, "Should not happen.");
            }
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        default:
          aptima_ASSERT(0, "Need to implement more types.");
          break;
      }
    } else {
      aptima_ASSERT(0, "Should not happen.");
    }
  } else {
    aptima_ASSERT(0, "Should not happen.");
  }

  goto done;

error:
  result = false;

done:
  return result;
}

bool aptima_msgpack_value_deserialize_inplace(aptima_value_t *value,
                                           msgpack_unpacker *unpacker,
                                           msgpack_unpacked *unpacked) {
  aptima_ASSERT(unpacker, "Invalid argument.");
  aptima_ASSERT(unpacked, "Invalid argument.");

  bool result = true;

  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_POSITIVE_INTEGER) {
      switch (MSGPACK_DATA_I64) {
        case aptima_TYPE_INT8: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_init_int8(value, MSGPACK_DATA_I64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_INT16: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_init_int16(value, MSGPACK_DATA_I64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_INT32: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_init_int32(value, MSGPACK_DATA_I64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_INT64: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_init_int64(value, MSGPACK_DATA_I64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_UINT8: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_init_uint8(value, MSGPACK_DATA_U64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_UINT16: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_init_uint16(value, MSGPACK_DATA_U64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_UINT32: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_init_uint32(value, MSGPACK_DATA_U64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_UINT64: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_init_uint64(value, MSGPACK_DATA_U64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_FLOAT32: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_init_float32(value, MSGPACK_DATA_F64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_FLOAT64: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_init_float64(value, MSGPACK_DATA_F64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_BOOL: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            aptima_value_init_bool(value, MSGPACK_DATA_I64);
            goto done;
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_STRING: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_STR) {
              aptima_value_init_string_with_size(value, MSGPACK_DATA_STR_PTR,
                                              MSGPACK_DATA_STR_SIZE);
              goto done;
            } else {
              aptima_ASSERT(0, "Should not happen.");
            }
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_BUF: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_BIN) {
              aptima_value_init_buf(value, 0);

              aptima_buf_t *buf = aptima_value_peek_buf(value);
              aptima_ASSERT(buf && aptima_buf_check_integrity(buf),
                         "Invalid argument.");

              aptima_buf_init_with_copying_data(
                  buf, (uint8_t *)MSGPACK_DATA_BIN_PTR, MSGPACK_DATA_BIN_SIZE);
              goto done;
            } else {
              aptima_ASSERT(0, "Should not happen.");
            }
          }
          break;
        }

        case aptima_TYPE_ARRAY: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_POSITIVE_INTEGER) {
              aptima_value_init_array_with_move(value, NULL);

              size_t array_items_cnt = MSGPACK_DATA_I64;
              for (size_t i = 0; i < array_items_cnt; i++) {
                aptima_value_t *item =
                    aptima_msgpack_create_value_through_deserialization(unpacker,
                                                                     unpacked);
                if (!item) {
                  goto error;
                }
                aptima_list_push_ptr_back(
                    &value->content.array, item,
                    (aptima_ptr_listnode_destroy_func_t)aptima_value_destroy);
              }

              goto done;
            } else {
              aptima_ASSERT(0, "Should not happen.");
            }
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        case aptima_TYPE_OBJECT: {
          msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
          if (rc == MSGPACK_UNPACK_SUCCESS) {
            if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_POSITIVE_INTEGER) {
              aptima_value_init_object_with_move(value, NULL);

              size_t object_kv_cnt = MSGPACK_DATA_I64;
              for (size_t i = 0; i < object_kv_cnt; i++) {
                aptima_value_kv_t *kv =
                    aptima_msgpack_create_value_kv_through_deserialization(
                        unpacker, unpacked);
                if (!kv) {
                  goto error;
                }
                aptima_list_push_ptr_back(
                    &value->content.object, kv,
                    (aptima_ptr_listnode_destroy_func_t)aptima_value_kv_destroy);
              }

              goto done;
            } else {
              aptima_ASSERT(0, "Should not happen.");
            }
          } else {
            aptima_ASSERT(0, "Should not happen.");
          }
          break;
        }

        default:
          aptima_ASSERT(0, "Need to implement more types.");
          break;
      }
    } else {
      aptima_ASSERT(0, "Should not happen.");
    }
  } else {
    aptima_ASSERT(0, "Should not happen.");
  }

  goto done;

error:
  aptima_value_deinit(value);
  result = false;

done:
  return result;
}

aptima_value_t *aptima_msgpack_create_value_through_deserialization(
    msgpack_unpacker *unpacker, msgpack_unpacked *unpacked) {
  aptima_value_t *result = aptima_value_create_invalid();
  if (aptima_msgpack_value_deserialize_inplace(result, unpacker, unpacked)) {
    return result;
  } else {
    aptima_value_destroy(result);
    return NULL;
  }
}

aptima_value_kv_t *aptima_msgpack_create_value_kv_through_deserialization(
    msgpack_unpacker *unpacker, msgpack_unpacked *unpacked) {
  aptima_ASSERT(unpacker, "Invalid argument.");
  aptima_ASSERT(unpacked, "Invalid argument.");

  aptima_value_kv_t *result = NULL;

  msgpack_unpack_return rc = msgpack_unpacker_next(unpacker, unpacked);
  if (rc == MSGPACK_UNPACK_SUCCESS) {
    if (MSGPACK_DATA_TYPE == MSGPACK_OBJECT_STR) {
      result = aptima_value_kv_create_vempty("%.*s", MSGPACK_DATA_STR_SIZE,
                                          MSGPACK_DATA_STR_PTR);
      result->value =
          aptima_msgpack_create_value_through_deserialization(unpacker, unpacked);
    } else {
      aptima_ASSERT(0, "Should not happen.");
    }
  } else {
    aptima_ASSERT(0, "Should not happen.");
  }

  return result;
}

void aptima_msgpack_value_serialize(aptima_value_t *value, msgpack_packer *pck) {
  aptima_ASSERT(value && aptima_value_check_integrity(value) && pck,
             "Invalid argument.");

  // Pack the type of value first.
  int rc = msgpack_pack_int32(pck, aptima_value_get_type(value));
  aptima_ASSERT(rc == 0, "Should not happen.");

  aptima_error_t err;
  aptima_error_init(&err);

  // Pack the data of value second.
  switch (aptima_value_get_type(value)) {
    case aptima_TYPE_INT8:
      rc = msgpack_pack_int8(pck, aptima_value_get_int8(value, &err));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_INT16:
      rc = msgpack_pack_int16(pck, aptima_value_get_int16(value, &err));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_INT32:
      rc = msgpack_pack_int32(pck, aptima_value_get_int32(value, &err));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_INT64:
      rc = msgpack_pack_int64(pck, aptima_value_get_int64(value, &err));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_UINT8:
      rc = msgpack_pack_uint8(pck, aptima_value_get_uint8(value, &err));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_UINT16:
      rc = msgpack_pack_uint16(pck, aptima_value_get_uint16(value, &err));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_UINT32:
      rc = msgpack_pack_uint32(pck, aptima_value_get_uint32(value, &err));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_UINT64:
      rc = msgpack_pack_uint64(pck, aptima_value_get_uint64(value, &err));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_FLOAT32:
      rc = msgpack_pack_float(pck, aptima_value_get_float32(value, &err));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_FLOAT64:
      rc = msgpack_pack_double(pck, aptima_value_get_float64(value, &err));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_STRING:
      rc = msgpack_pack_str_with_body(
          pck, aptima_value_peek_raw_str(value, &err),
          strlen(aptima_value_peek_raw_str(value, &err)));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_BOOL:
      rc = msgpack_pack_int32(pck, aptima_value_get_bool(value, &err));
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_BUF:
      rc = msgpack_pack_bin_with_body(pck, aptima_value_peek_buf(value)->data,
                                      aptima_value_peek_buf(value)->size);
      aptima_ASSERT(rc == 0, "Should not happen.");
      break;
    case aptima_TYPE_ARRAY:
      // Pack array size first.
      //
      // Note: We can _not_ use msgpack_pack_array() here, because the
      // value_kv itself contains many fields, so that many msgpack_objects
      // would be generated during the serialization of the value_kv. And that
      // is not a correct way to use msgpack, what the `n` in
      // `msgpack_pack_array(, n)` means is the msgpack_objects in the map. So
      // only if we can know how many msgpack_objects will be generated during
      // the serialization of the value_kv, we can use msgpack_pack_array()
      // here. However, it would be very difficult to know this in advance, so
      // we directly pack the count number here, and let the unpacker to
      // unpack this numbers of value_kv later.
      rc = msgpack_pack_uint32(pck, aptima_list_size(&value->content.array));
      aptima_ASSERT(rc == 0, "Should not happen.");

      // Pack data second.
      aptima_list_foreach (&value->content.array, iter) {
        aptima_value_t *array_item = aptima_ptr_listnode_get(iter.node);
        aptima_ASSERT(array_item && aptima_value_check_integrity(array_item),
                   "Invalid argument.");
        aptima_msgpack_value_serialize(array_item, pck);
      }
      break;
    case aptima_TYPE_OBJECT:
      // Pack map size first.
      //
      // Note: We can _not_ use msgpack_pack_map() here, because the value_kv
      // itself contains many fields, so that many msgpack_objects would be
      // generated during the serialization of the value_kv. And that is not a
      // correct way to use msgpack, what the `n` in `msgpack_pack_map(, n)`
      // means is the msgpack_objects in the map. So only if we can know how
      // many msgpack_objects will be generated during the serialization of
      // the value_kv, we can use msgpack_pack_map() here. However, it would
      // be very difficult to know this in advance, so we directly pack the
      // count number here, and let the unpacker to unpack this numbers of
      // value_kv later.
      rc = msgpack_pack_uint32(pck, aptima_list_size(&value->content.object));
      aptima_ASSERT(rc == 0, "Should not happen.");

      // Pack data second.
      aptima_list_foreach (&value->content.object, iter) {
        aptima_value_kv_t *object_item = aptima_ptr_listnode_get(iter.node);
        aptima_ASSERT(object_item && aptima_value_kv_check_integrity(object_item),
                   "Invalid argument.");
        aptima_msgpack_value_kv_serialize(object_item, pck);
      }
      break;
    default:
      aptima_ASSERT(0, "Need to implement more types (%d)",
                 aptima_value_get_type(value));
      break;
  }

  aptima_error_deinit(&err);
}

void aptima_msgpack_value_kv_serialize(aptima_value_kv_t *kv, msgpack_packer *pck) {
  aptima_ASSERT(kv && aptima_value_kv_check_integrity(kv) && pck,
             "Invalid argument.");

  int rc = msgpack_pack_str_with_body(
      pck, aptima_string_get_raw_str(aptima_value_kv_get_key(kv)),
      aptima_string_len(aptima_value_kv_get_key(kv)));
  aptima_ASSERT(rc == 0, "Should not happen.");

  aptima_msgpack_value_serialize(kv->value, pck);
}
