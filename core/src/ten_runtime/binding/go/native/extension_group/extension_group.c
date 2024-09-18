//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/binding/go/interface/ten/extension_group.h"

#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/extension/extension.h"
#include "include_internal/ten_runtime/binding/go/extension_group/extension_group.h"
#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "ten_runtime/binding/common.h"
#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"

void tenGoExtensionGroupOnInit(ten_go_handle_t go_extension_group,
                               ten_go_handle_t go_ten);

void tenGoExtensionGroupOnDeinit(ten_go_handle_t go_extension_group,
                                 ten_go_handle_t go_ten);

void tenGoExtensionGroupOnCreateExtensions(ten_go_handle_t go_extension_group,
                                           ten_go_handle_t go_ten);

void tenGoExtensionGroupOnDestroyExtensions(
    ten_go_handle_t go_extension_group, ten_go_handle_t go_ten,
    ten_go_handle_array_t *extension_array);

bool ten_go_extension_group_check_integrity(ten_go_extension_group_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) != TEN_GO_EXTENSION_GROUP_SIGNATURE) {
    return false;
  }

  return true;
}

ten_go_extension_group_t *ten_go_extension_group_reinterpret(
    uintptr_t extension_group_bridge) {
  TEN_ASSERT(extension_group_bridge > 0, "Invalid argument.");

  ten_go_extension_group_t *self =
      // NOLINTNEXTLINE(performance-no-int-to-ptr)
      (ten_go_extension_group_t *)extension_group_bridge;
  TEN_ASSERT(ten_go_extension_group_check_integrity(self), "Invalid argument.");

  return self;
}

ten_go_handle_t ten_go_extension_group_go_handle(
    ten_go_extension_group_t *self) {
  TEN_ASSERT(ten_go_extension_group_check_integrity(self),
             "Should not happen.");

  return self->bridge.go_instance;
}

ten_extension_group_t *ten_go_extension_group_c_extension_group(
    ten_go_extension_group_t *self) {
  TEN_ASSERT(ten_go_extension_group_check_integrity(self),
             "Should not happen.");
  return self->c_extension_group;
}

static void ten_go_extension_group_bridge_destroy(
    ten_go_extension_group_t *self) {
  TEN_ASSERT(ten_go_extension_group_check_integrity(self),
             "Should not happen.");
  TEN_ASSERT(self->c_extension_group && ten_extension_group_check_integrity(
                                            self->c_extension_group, false),
             "Should not happen.");

  ten_extension_group_destroy(self->c_extension_group);
  TEN_FREE(self);
}

static void proxy_on_init(ten_extension_group_t *self, ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_extension_group_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(self->ten_env == ten_env, "Should not happen.");

  ten_go_extension_group_t *extension_group_bridge =
      ten_binding_handle_get_me_in_target_lang((ten_binding_handle_t *)self);
  TEN_ASSERT(ten_go_extension_group_check_integrity(extension_group_bridge),
             "Should not happen.");

  ten_go_ten_env_t *ten_bridge = ten_go_ten_env_wrap(ten_env);
  ten_bridge->c_ten_env_proxy = ten_env_proxy_create(ten_env, 1, NULL);

  tenGoExtensionGroupOnInit(extension_group_bridge->bridge.go_instance,
                            ten_go_ten_env_go_handle(ten_bridge));
}

static void proxy_on_deinit(ten_extension_group_t *self, ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_extension_group_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(self->ten_env == ten_env, "Should not happen.");

  ten_go_extension_group_t *extension_group_bridge =
      ten_binding_handle_get_me_in_target_lang((ten_binding_handle_t *)self);
  TEN_ASSERT(ten_go_extension_group_check_integrity(extension_group_bridge),
             "Should not happen.");

  ten_go_ten_env_t *ten_bridge = ten_go_ten_env_wrap(ten_env);

  tenGoExtensionGroupOnDeinit(extension_group_bridge->bridge.go_instance,
                              ten_go_ten_env_go_handle(ten_bridge));
}

static void proxy_on_create_extensions(ten_extension_group_t *self,
                                       ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_extension_group_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(self->ten_env == ten_env, "Should not happen.");

  ten_go_extension_group_t *extension_group_bridge =
      ten_binding_handle_get_me_in_target_lang((ten_binding_handle_t *)self);
  TEN_ASSERT(ten_go_extension_group_check_integrity(extension_group_bridge),
             "Should not happen.");

  ten_go_ten_env_t *ten_bridge = ten_go_ten_env_wrap(ten_env);

  tenGoExtensionGroupOnCreateExtensions(
      extension_group_bridge->bridge.go_instance,
      ten_go_ten_env_go_handle(ten_bridge));
}

static void proxy_on_destroy_extensions(ten_extension_group_t *self,
                                        ten_env_t *ten_env,
                                        ten_list_t extensions) {
  TEN_ASSERT(self && ten_extension_group_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");
  TEN_ASSERT(self->ten_env == ten_env, "Should not happen.");

  ten_go_extension_group_t *extension_group_bridge =
      ten_binding_handle_get_me_in_target_lang((ten_binding_handle_t *)self);
  TEN_ASSERT(ten_go_extension_group_check_integrity(extension_group_bridge),
             "Should not happen.");

  ten_go_ten_env_t *ten_bridge = ten_go_ten_env_wrap(ten_env);

  ten_go_handle_array_t *extensions_array =
      ten_go_handle_array_create(ten_list_size(&extensions));

  int i = 0;
  ten_list_foreach (&extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
               "Should not happen.");

    ten_go_extension_t *extension_bridge =
        ten_binding_handle_get_me_in_target_lang(
            (ten_binding_handle_t *)extension);
    TEN_ASSERT(
        extension_bridge && ten_go_extension_check_integrity(extension_bridge),
        "Should not happen.");

    ten_go_handle_t go_extension = ten_go_extension_go_handle(extension_bridge);
    extensions_array->array[i++] = go_extension;
  }

  tenGoExtensionGroupOnDestroyExtensions(
      extension_group_bridge->bridge.go_instance,
      ten_go_ten_env_go_handle(ten_bridge), extensions_array);

  ten_go_handle_array_destroy(extensions_array);
}

ten_go_extension_group_t *ten_go_extension_group_create_internal(
    ten_go_handle_t go_extension_group, const char *name) {
  ten_go_extension_group_t *extension_group_bridge =
      (ten_go_extension_group_t *)TEN_MALLOC(sizeof(ten_go_extension_group_t));
  TEN_ASSERT(extension_group_bridge, "Failed to allocate memory.");

  ten_signature_set(&extension_group_bridge->signature,
                    TEN_GO_EXTENSION_GROUP_SIGNATURE);
  extension_group_bridge->bridge.go_instance = go_extension_group;

  // The extension group bridge instance is created and managed only by Go. When
  // the Go extension group is finalized, the extension group bridge instance
  // will be destroyed. Therefore, the C part should not hold any reference to
  // the extension group bridge instance.
  extension_group_bridge->bridge.sp_ref_by_go = ten_shared_ptr_create(
      extension_group_bridge, ten_go_extension_group_bridge_destroy);
  extension_group_bridge->bridge.sp_ref_by_c = NULL;

  extension_group_bridge->c_extension_group = ten_extension_group_create(
      name, NULL, proxy_on_init, proxy_on_deinit, proxy_on_create_extensions,
      proxy_on_destroy_extensions);

  ten_binding_handle_set_me_in_target_lang(
      (ten_binding_handle_t *)(extension_group_bridge->c_extension_group),
      extension_group_bridge);

  return extension_group_bridge;
}

ten_go_status_t ten_go_extension_group_create(
    ten_go_handle_t go_extension_group_index, const void *name, int name_len,
    uintptr_t *bridge_addr) {
  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  ten_string_t extension_group_name;
  ten_string_init_formatted(&extension_group_name, "%.*s", name_len, name);

  ten_go_extension_group_t *self = ten_go_extension_group_create_internal(
      go_extension_group_index, ten_string_get_raw_str(&extension_group_name));

  ten_string_deinit(&extension_group_name);

  *bridge_addr = (uintptr_t)self;

  return status;
}

void ten_go_extension_group_finalize(uintptr_t bridge_addr) {
  ten_go_extension_group_t *self =
      ten_go_extension_group_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_extension_group_check_integrity(self),
             "Should not happen.");
  ten_go_bridge_destroy_go_part(&self->bridge);
}

void ten_go_extension_group_set_go_handle(ten_go_extension_group_t *self,
                                          ten_go_handle_t go_handle) {
  TEN_ASSERT(self && ten_go_extension_group_check_integrity(self),
             "Invalid argument.");

  self->bridge.go_instance = go_handle;
}
