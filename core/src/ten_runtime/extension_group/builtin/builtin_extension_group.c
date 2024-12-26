//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdio.h>
#include <stdlib.h>

#include "include_internal/ten_runtime/addon/addon.h"
#include "include_internal/ten_runtime/addon/extension_group/extension_group.h"
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/extension/extension_addon_and_instance_name_pair.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/ten_env/metadata.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/addon/addon.h"
#include "ten_runtime/ten.h"
#include "ten_runtime/ten_env/internal/log.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

// Because the creation process for extensions is asynchronous, it is necessary
// to check whether the number of extensions already created has reached the
// initially set target each time an extension is successfully created. If the
// target is met, it means that all the required extensions for this extension
// group have been successfully created.
static void on_addon_create_extension_done(ten_env_t *ten_env,
                                           ten_extension_t *extension,
                                           void *cb_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Invalid argument.");
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Invalid argument.");
  TEN_ASSERT(
      ten_env_get_attach_to(ten_env) == TEN_ENV_ATTACH_TO_EXTENSION_GROUP,
      "Invalid argument.");

  ten_extension_group_t *extension_group = ten_env_get_attached_target(ten_env);
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Invalid argument.");

  ten_list_t *result = (ten_list_t *)cb_data;
  TEN_ASSERT(result, "Should not happen.");

  ten_list_push_ptr_back(result, extension, NULL);

  if (ten_list_size(result) ==
      ten_list_size(
          ten_extension_group_get_extension_addon_and_instance_name_pairs(
              extension_group))) {
    // Notify the builtin extension group that all extensions have been created.
    ten_env_on_create_extensions_done(
        ten_extension_group_get_ten_env(extension_group), result, NULL);

    ten_list_destroy(result);
  }
}

static void on_addon_destroy_instance_done(ten_env_t *ten_env,
                                           TEN_UNUSED void *cb_data) {
  TEN_ASSERT(ten_env && ten_env_check_integrity(ten_env, true),
             "Invalid argument.");
  TEN_ASSERT(
      ten_env_get_attach_to(ten_env) == TEN_ENV_ATTACH_TO_EXTENSION_GROUP,
      "Invalid argument.");

  ten_extension_group_t *extension_group = ten_env_get_attached_target(ten_env);
  TEN_ASSERT(extension_group &&
                 ten_extension_group_check_integrity(extension_group, true),
             "Invalid argument.");

  // We modify 'extensions_cnt_of_being_destroyed' on the extension thread, so
  // it's thread safe.
  if (ten_extension_group_decrement_extension_cnt_of_being_destroyed(
          extension_group) == 0) {
    ten_env_on_destroy_extensions_done(ten_env, NULL);
  }
}

static void ten_builtin_extension_group_on_init(ten_extension_group_t *self,
                                                ten_env_t *ten_env) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env, "Invalid argument.");

  ten_env_on_init_done(ten_env, NULL);
}

static void ten_builtin_extension_group_on_deinit(ten_extension_group_t *self,
                                                  ten_env_t *ten_env) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env, "Invalid argument.");

  ten_env_on_deinit_done(ten_env, NULL);
}

static void ten_builtin_extension_group_on_create_extensions(
    ten_extension_group_t *self, ten_env_t *ten_env) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env, "Invalid argument.");

  ten_list_t *result = ten_list_create();

  if (ten_list_is_empty(
          ten_extension_group_get_extension_addon_and_instance_name_pairs(
              self))) {
    ten_env_on_create_extensions_done(ten_env, result, NULL);
    ten_list_destroy(result);
    return;
  }

  // Get the information of all the extensions which this extension group should
  // create.
  ten_list_foreach (
      ten_extension_group_get_extension_addon_and_instance_name_pairs(self),
      iter) {
    ten_extension_addon_and_instance_name_pair_t *extension_name_info =
        ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(extension_name_info, "Invalid argument.");

    ten_string_t *extension_addon_name = &extension_name_info->addon_name;
    ten_string_t *extension_instance_name = &extension_name_info->instance_name;

    bool res = ten_addon_create_extension(
        ten_env, ten_string_get_raw_str(extension_addon_name),
        ten_string_get_raw_str(extension_instance_name),
        (ten_env_addon_create_instance_done_cb_t)on_addon_create_extension_done,
        result, NULL);

    if (!res) {
      TEN_LOGE("Failed to find the addon for extension %s",
               ten_string_get_raw_str(extension_addon_name));
#if defined(_DEBUG)
      TEN_ASSERT(0, "Should not happen.");
#endif
    }
  }
}

static void ten_builtin_extension_group_on_destroy_extensions(
    ten_extension_group_t *self, ten_env_t *ten_env, ten_list_t extensions) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env, "Invalid argument.");

  if (ten_list_size(&extensions) == 0) {
    ten_env_on_destroy_extensions_done(ten_env, NULL);
    return;
  }

  ten_extension_group_set_extension_cnt_of_being_destroyed(
      self, ten_list_size(&extensions));

  ten_list_foreach (&extensions, iter) {
    ten_extension_t *extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
               "Invalid argument.");

    ten_addon_destroy_extension(ten_env, extension,
                                on_addon_destroy_instance_done, NULL, NULL);
  }
}

void ten_builtin_extension_group_addon_on_init(TEN_UNUSED ten_addon_t *addon,
                                               ten_env_t *ten_env) {
  bool result = ten_env_init_manifest_from_json(ten_env,
                                                // clang-format off
                            "{\
                              \"type\": \"extension_group\",\
                              \"name\": \"default_extension_group\",\
                              \"version\": \"1.0.0\"\
                             }",
                                                // clang-format on
                                                NULL);
  TEN_ASSERT(result, "Should not happen.");

  ten_env_on_init_done(ten_env, NULL);
}

void ten_builtin_extension_group_addon_create_instance(ten_addon_t *addon,
                                                       ten_env_t *ten_env,
                                                       const char *name,
                                                       void *context) {
  TEN_ASSERT(addon && name, "Invalid argument.");

  ten_extension_group_t *ext_group = ten_extension_group_create(
      name, NULL, ten_builtin_extension_group_on_init,
      ten_builtin_extension_group_on_deinit,
      ten_builtin_extension_group_on_create_extensions,
      ten_builtin_extension_group_on_destroy_extensions);

  ten_env_on_create_instance_done(ten_env, ext_group, context, NULL);
}

void ten_builtin_extension_group_addon_destroy_instance(
    TEN_UNUSED ten_addon_t *addon, ten_env_t *ten_env, void *_extension_group,
    void *context) {
  ten_extension_group_t *extension_group =
      (ten_extension_group_t *)_extension_group;
  TEN_ASSERT(extension_group, "Invalid argument.");

  ten_extension_group_destroy(extension_group);

  ten_env_on_destroy_instance_done(ten_env, context, NULL);
}

static ten_addon_t builtin_extension_group_addon = {
    NULL,
    TEN_ADDON_SIGNATURE,
    ten_builtin_extension_group_addon_on_init,
    NULL,
    ten_builtin_extension_group_addon_create_instance,
    ten_builtin_extension_group_addon_destroy_instance,
    NULL,
    NULL,
};

void ten_builtin_extension_group_addon_register(void) {
  ten_addon_register_extension_group(TEN_STR_DEFAULT_EXTENSION_GROUP, NULL,
                                     &builtin_extension_group_addon, NULL);
}

void ten_builtin_extension_group_addon_unregister(void) {
  ten_addon_unregister_extension_group(TEN_STR_DEFAULT_EXTENSION_GROUP);
}
