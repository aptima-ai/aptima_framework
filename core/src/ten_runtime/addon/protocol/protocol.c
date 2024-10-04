//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/addon/protocol/protocol.h"

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/common/store.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/lib/uri.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_get.h"
#include "ten_utils/value/value_object.h"

static ten_addon_store_t g_protocol_store = {
    false,
    NULL,
    TEN_LIST_INIT_VAL,
};

ten_addon_store_t *ten_protocol_get_store(void) { return &g_protocol_store; }

ten_addon_t *ten_addon_unregister_protocol(const char *name) {
  TEN_ASSERT(name, "Should not happen.");

  return ten_addon_unregister(ten_protocol_get_store(), name);
}

void ten_addon_register_protocol(const char *name, const char *base_dir,
                                 ten_addon_t *addon) {
  ten_addon_store_init(ten_protocol_get_store());

  ten_addon_host_t *addon_host =
      (ten_addon_host_t *)TEN_MALLOC(sizeof(ten_addon_host_t));
  TEN_ASSERT(addon_host, "Failed to allocate memory.");

  addon_host->type = TEN_ADDON_TYPE_PROTOCOL;
  ten_addon_host_init(addon_host);

  ten_addon_register(ten_protocol_get_store(), addon_host, name, base_dir,
                     addon);
}

static bool ten_addon_protocol_match_protocol(ten_addon_host_t *self,
                                              const char *protocol) {
  TEN_ASSERT(self && self->type == TEN_ADDON_TYPE_PROTOCOL && protocol,
             "Should not happen.");

  ten_value_t *manifest = &self->manifest;
  TEN_ASSERT(manifest, "Invalid argument.");
  TEN_ASSERT(ten_value_check_integrity(manifest), "Invalid use of manifest %p.",
             manifest);

  bool found = false;
  ten_list_t *addon_protocols =
      ten_value_object_peek_array(manifest, TEN_STR_PROTOCOL);
  TEN_ASSERT(addon_protocols, "Should not happen.");

  ten_list_foreach (addon_protocols, iter) {
    ten_value_t *addon_protocol_value = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(
        addon_protocol_value && ten_value_check_integrity(addon_protocol_value),
        "Should not happen.");

    const char *addon_protocol = ten_value_peek_string(addon_protocol_value);
    if (!strcmp(addon_protocol, protocol)) {
      found = true;
      break;
    }
  }

  return found;
}

ten_addon_host_t *ten_addon_protocol_find(const char *protocol) {
  TEN_ASSERT(protocol, "Should not happen.");

  ten_addon_host_t *result = NULL;

  ten_addon_store_t *store = ten_protocol_get_store();
  TEN_ASSERT(store, "Should not happen.");

  ten_mutex_lock(store->lock);

  ten_list_foreach (&store->store, iter) {
    ten_addon_host_t *addon = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(addon && addon->type == TEN_ADDON_TYPE_PROTOCOL,
               "Should not happen.");

    if (!ten_addon_protocol_match_protocol(addon, protocol)) {
      continue;
    }

    result = addon;
    break;
  }

  ten_mutex_unlock(store->lock);

  return result;
}
