//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/msg_dest_info/msg_dest_info.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/extension/extension_info/json.h"
#include "axis_utils/container/list_node_smart_ptr.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

bool axis_msg_dest_info_check_integrity(axis_msg_dest_info_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_MSG_DEST_STATIC_INFO_SIGNATURE) {
    return false;
  }
  return true;
}

axis_msg_dest_info_t *axis_msg_dest_info_create(const char *msg_name) {
  axis_ASSERT(msg_name, "Should not happen.");

  axis_msg_dest_info_t *self =
      (axis_msg_dest_info_t *)axis_MALLOC(sizeof(axis_msg_dest_info_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_MSG_DEST_STATIC_INFO_SIGNATURE);
  axis_list_init(&self->dest);

  axis_string_init_formatted(&self->name, "%s", msg_name);

  self->policy = axis_DEFAULT_RESULT_RETURN_POLICY;

  return self;
}

void axis_msg_dest_info_destroy(axis_msg_dest_info_t *self) {
  axis_ASSERT(self && axis_msg_dest_info_check_integrity(self),
             "Should not happen.");

  axis_signature_set(&self->signature, 0);

  axis_list_clear(&self->dest);
  axis_string_deinit(&self->name);

  axis_FREE(self);
}

axis_shared_ptr_t *axis_msg_dest_info_clone(axis_shared_ptr_t *self,
                                          axis_list_t *extensions_info,
                                          axis_error_t *err) {
  axis_ASSERT(self && extensions_info, "Should not happen.");

  axis_msg_dest_info_t *msg_dest_info = axis_shared_ptr_get_data(self);
  axis_ASSERT(msg_dest_info && axis_msg_dest_info_check_integrity(msg_dest_info),
             "Should not happen.");

  const char *msg_name = axis_string_get_raw_str(&msg_dest_info->name);

  axis_msg_dest_info_t *new_self = axis_msg_dest_info_create(msg_name);

  axis_list_foreach (&msg_dest_info->dest, iter) {
    axis_weak_ptr_t *dest = axis_smart_ptr_listnode_get(iter.node);
    axis_extension_info_t *dest_extension_info =
        axis_extension_info_from_smart_ptr(dest);

    axis_shared_ptr_t *new_dest = get_extension_info_in_extensions_info(
        extensions_info,
        axis_string_get_raw_str(&dest_extension_info->loc.app_uri),
        axis_string_get_raw_str(&dest_extension_info->loc.graph_id),
        axis_string_get_raw_str(&dest_extension_info->loc.extension_group_name),
        NULL, axis_string_get_raw_str(&dest_extension_info->loc.extension_name),
        true, err);
    axis_ASSERT(new_dest, "Should not happen.");

    if (!new_dest) {
      return NULL;
    }

    // We need to use weak_ptr here to prevent the circular shared_ptr problem
    // in the case of loop graph.
    axis_weak_ptr_t *weak_dest = axis_weak_ptr_create(new_dest);
    axis_list_push_smart_ptr_back(&new_self->dest, weak_dest);
    axis_weak_ptr_destroy(weak_dest);
  }

  return axis_shared_ptr_create(new_self, axis_msg_dest_info_destroy);
}

void axis_msg_dest_info_translate_localhost_to_app_uri(axis_msg_dest_info_t *self,
                                                      const char *uri) {
  axis_ASSERT(self && uri, "Should not happen.");

  axis_list_foreach (&self->dest, iter) {
    axis_shared_ptr_t *shared_dest =
        axis_weak_ptr_lock(axis_smart_ptr_listnode_get(iter.node));

    axis_extension_info_t *extension_info = axis_shared_ptr_get_data(shared_dest);
    axis_extension_info_translate_localhost_to_app_uri(extension_info, uri);

    axis_shared_ptr_destroy(shared_dest);
  }
}

bool axis_msg_dest_info_qualified(axis_msg_dest_info_t *self,
                                 const char *msg_name) {
  axis_ASSERT(self && axis_msg_dest_info_check_integrity(self) && msg_name,
             "Should not happen.");

  if (axis_c_string_is_equal(axis_string_get_raw_str(&self->name), msg_name)) {
    return true;
  }

  // "*" is a special rule that matches all names.
  if (axis_string_is_equal_c_str(&self->name, axis_STR_STAR)) {
    return true;
  }

  return false;
}
