//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension/msg_dest_info/value.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension/extension_info/extension_info.h"
#include "include_internal/ten_runtime/extension/extension_info/value.h"
#include "include_internal/ten_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "ten_utils/macro/check.h"

// Parse the following snippet.
//
// ------------------------
// "name": "...",
// "dest": [{
//   "app": "...",
//   "extension_group": "...",
//   "extension": "...",
//   "msg_conversion": {
//   }
// }]
// ------------------------
ten_shared_ptr_t *ten_msg_dest_static_info_from_value(
    ten_value_t *value, ten_list_t *extensions_info,
    ten_extension_info_t *src_extension_info, ten_error_t *err) {
  TEN_ASSERT(value && extensions_info, "Should not happen.");
  TEN_ASSERT(src_extension_info,
             "src_extension must be specified in this case.");

  ten_msg_dest_static_info_t *self = NULL;

  // "name": "...",
  ten_value_t *name_value = ten_value_object_peek(value, TEN_STR_NAME);

  const char *name = "";
  if (name_value) {
    name = ten_value_peek_c_str(name_value);
  }

  self = ten_msg_dest_static_info_create(name);
  TEN_ASSERT(self, "Should not happen.");

  // "dest": [{
  //   "app": "...",
  //   "extension_group": "...",
  //   "extension": "...",
  //   "msg_conversion": {
  //   }
  // }]
  ten_value_t *dests_value = ten_value_object_peek(value, TEN_STR_DEST);
  if (dests_value) {
    if (!ten_value_is_array(dests_value)) {
      goto error;
    }

    ten_value_array_foreach(dests_value, iter) {
      ten_value_t *dest_value = ten_ptr_listnode_get(iter.node);
      if (!ten_value_is_object(dest_value)) {
        goto error;
      }

      ten_shared_ptr_t *dest =
          ten_extension_info_parse_connection_dest_part_from_value(
              dest_value, extensions_info, src_extension_info, name, err);
      if (!dest) {
        goto error;
      }

      // We need to use weak_ptr here to prevent the circular shared_ptr problem
      // in the case of loop graph.
      ten_weak_ptr_t *weak_dest = ten_weak_ptr_create(dest);
      ten_list_push_smart_ptr_back(&self->dest, weak_dest);
      ten_weak_ptr_destroy(weak_dest);
    }
  }

  goto done;

error:
  if (!self) {
    ten_msg_dest_static_info_destroy(self);
    self = NULL;
  }

done:
  if (self) {
    return ten_shared_ptr_create(self, ten_msg_dest_static_info_destroy);
  } else {
    return NULL;
  }
}
