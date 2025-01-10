//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_group/builtin/builtin_extension_group.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/addon/extension/extension.h"
#include "include_internal/axis_runtime/addon/extension_group/extension_group.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/extension_addon_and_instance_name_pair.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/axis_env/metadata.h"
#include "include_internal/axis_runtime/axis_env/on_xxx_done.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/addon/addon.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/msg/msg.h"
#include "axis_runtime/aptima.h"
#include "axis_runtime/axis_env/internal/log.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/container/list_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"

axis_extension_group_create_extensions_done_ctx_t *
axis_extension_group_create_extensions_done_ctx_create(void) {
  axis_extension_group_create_extensions_done_ctx_t *self =
      axis_MALLOC(sizeof(axis_extension_group_create_extensions_done_ctx_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_list_init(&self->results);

  return self;
}

void axis_extension_group_create_extensions_done_ctx_destroy(
    axis_extension_group_create_extensions_done_ctx_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_list_clear(&self->results);

  axis_FREE(self);
}

// Because the creation process for extensions is asynchronous, it is necessary
// to check whether the number of extensions already created has reached the
// initially set target each time an extension is successfully created. If the
// target is met, it means that all the required extensions for this extension
// group have been successfully created.
static void on_addon_create_extension_done(axis_env_t *axis_env,
                                           axis_extension_t *extension,
                                           void *cb_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");
  axis_ASSERT(
      axis_env_get_attach_to(axis_env) == axis_ENV_ATTACH_TO_EXTENSION_GROUP,
      "Invalid argument.");

  axis_addon_create_extension_done_ctx_t *create_extension_done_ctx =
      (axis_addon_create_extension_done_ctx_t *)cb_data;
  axis_ASSERT(create_extension_done_ctx, "Should not happen.");

  axis_extension_group_create_extensions_done_ctx_t *create_extensions_done_ctx =
      create_extension_done_ctx->create_extensions_done_ctx;

  axis_list_t *results = &create_extensions_done_ctx->results;
  axis_ASSERT(results, "Should not happen.");

  axis_extension_group_t *extension_group = axis_env_get_attached_target(axis_env);
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Invalid argument.");

  if (extension) {
    // Successful to create the specified extension.

    axis_LOGI(
        "Success to create extension %s",
        axis_string_get_raw_str(&create_extension_done_ctx->extension_name));

    axis_ASSERT(axis_extension_check_integrity(extension, true),
               "Invalid argument.");

    axis_list_push_ptr_back(results, extension, NULL);
  } else {
    // Failed to create the specified extension.

    axis_LOGI(
        "Failed to create extension %s",
        axis_string_get_raw_str(&create_extension_done_ctx->extension_name));

    // Use a value that is absolutely incorrect to represent an extension that
    // could not be successfully created. This ensures that the final count in
    // the `results` matches the expected number; otherwise, it would get stuck,
    // endlessly waiting for the desired number of extensions to be created. In
    // later steps, these special, unsuccessfully created extension instances
    // will be removed.
    axis_list_push_ptr_back(results, axis_EXTENSION_UNSUCCESSFULLY_CREATED, NULL);
  }

  if (axis_list_size(results) ==
      axis_list_size(
          axis_extension_group_get_extension_addon_and_instance_name_pairs(
              extension_group))) {
    // Notify the builtin extension group that all extensions have been created.

    axis_env_on_create_extensions_done(
        axis_extension_group_get_axis_env(extension_group),
        create_extensions_done_ctx, NULL);

    axis_extension_group_create_extensions_done_ctx_destroy(
        create_extensions_done_ctx);
  }

  axis_addon_create_extension_done_ctx_destroy(create_extension_done_ctx);
}

static void on_addon_destroy_instance_done(axis_env_t *axis_env,
                                           axis_UNUSED void *cb_data) {
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Invalid argument.");
  axis_ASSERT(
      axis_env_get_attach_to(axis_env) == axis_ENV_ATTACH_TO_EXTENSION_GROUP,
      "Invalid argument.");

  axis_extension_group_t *extension_group = axis_env_get_attached_target(axis_env);
  axis_ASSERT(extension_group &&
                 axis_extension_group_check_integrity(extension_group, true),
             "Invalid argument.");

  // We modify 'extensions_cnt_of_being_destroyed' on the extension thread, so
  // it's thread safe.
  if (axis_extension_group_decrement_extension_cnt_of_being_destroyed(
          extension_group) == 0) {
    axis_env_on_destroy_extensions_done(axis_env, NULL);
  }
}

static void axis_builtin_extension_group_on_init(axis_extension_group_t *self,
                                                axis_env_t *axis_env) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env, "Invalid argument.");

  axis_env_on_init_done(axis_env, NULL);
}

static void axis_builtin_extension_group_on_deinit(axis_extension_group_t *self,
                                                  axis_env_t *axis_env) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env, "Invalid argument.");

  axis_env_on_deinit_done(axis_env, NULL);
}

static void axis_builtin_extension_group_on_create_extensions(
    axis_extension_group_t *self, axis_env_t *axis_env) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env, "Invalid argument.");

  axis_extension_group_create_extensions_done_ctx_t *create_extensions_done_ctx =
      axis_extension_group_create_extensions_done_ctx_create();

  if (axis_list_is_empty(
          axis_extension_group_get_extension_addon_and_instance_name_pairs(
              self))) {
    // This extension group is empty, so it can be considered that all the
    // required extensions have been successfully created.

    axis_LOGI(
        "%s is a group without any extensions, so it is considered that all "
        "extensions have been successfully created.",
        axis_string_get_raw_str(&self->name));

    axis_env_on_create_extensions_done(axis_env, create_extensions_done_ctx,
                                      NULL);
    axis_extension_group_create_extensions_done_ctx_destroy(
        create_extensions_done_ctx);

    return;
  }

  // Get the information of all the extensions which this extension group should
  // create.
  axis_list_foreach (
      axis_extension_group_get_extension_addon_and_instance_name_pairs(self),
      iter) {
    axis_extension_addon_and_instance_name_pair_t *extension_name_info =
        axis_ptr_listnode_get(iter.node);
    axis_ASSERT(extension_name_info, "Invalid argument.");

    axis_string_t *extension_addon_name = &extension_name_info->addon_name;
    axis_string_t *extension_instance_name = &extension_name_info->instance_name;

    axis_addon_create_extension_done_ctx_t *create_extension_done_ctx =
        axis_addon_create_extension_done_ctx_create(
            axis_string_get_raw_str(extension_instance_name),
            create_extensions_done_ctx);

    bool res = axis_addon_create_extension(
        axis_env, axis_string_get_raw_str(extension_addon_name),
        axis_string_get_raw_str(extension_instance_name),
        (axis_env_addon_create_instance_done_cb_t)on_addon_create_extension_done,
        create_extension_done_ctx, NULL);

    if (!res) {
      axis_LOGE("Failed to find the addon for extension %s",
               axis_string_get_raw_str(extension_addon_name));

      axis_error_set(&self->err_before_ready, axis_ERRNO_INVALID_GRAPH,
                    "Failed to find the addon for extension %s",
                    axis_string_get_raw_str(extension_addon_name));

      // Unable to create the desired extension, proceeding with the failure
      // path. The callback of `axis_addon_create_extension` will not be invoked
      // when `res` is `false`, so we need to call the callback function here to
      // ensure the process can continue.
      on_addon_create_extension_done(axis_env, NULL, create_extension_done_ctx);
    }
  }
}

static void axis_builtin_extension_group_on_destroy_extensions(
    axis_extension_group_t *self, axis_env_t *axis_env, axis_list_t extensions) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_env, "Invalid argument.");

  if (axis_list_size(&extensions) == 0) {
    axis_env_on_destroy_extensions_done(axis_env, NULL);
    return;
  }

  axis_extension_group_set_extension_cnt_of_being_destroyed(
      self, axis_list_size(&extensions));

  axis_list_foreach (&extensions, iter) {
    axis_extension_t *extension = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
               "Invalid argument.");

    axis_addon_destroy_extension(axis_env, extension,
                                on_addon_destroy_instance_done, NULL, NULL);
  }
}

void axis_builtin_extension_group_addon_on_init(axis_UNUSED axis_addon_t *addon,
                                               axis_env_t *axis_env) {
  bool result = axis_env_init_manifest_from_json(axis_env,
                                                // clang-format off
                            "{\
                              \"type\": \"extension_group\",\
                              \"name\": \"default_extension_group\",\
                              \"version\": \"1.0.0\"\
                             }",
                                                // clang-format on
                                                NULL);
  axis_ASSERT(result, "Should not happen.");

  axis_env_on_init_done(axis_env, NULL);
}

void axis_builtin_extension_group_addon_create_instance(axis_addon_t *addon,
                                                       axis_env_t *axis_env,
                                                       const char *name,
                                                       void *context) {
  axis_ASSERT(addon && name, "Invalid argument.");

  axis_extension_group_t *ext_group = axis_extension_group_create(
      name, NULL, axis_builtin_extension_group_on_init,
      axis_builtin_extension_group_on_deinit,
      axis_builtin_extension_group_on_create_extensions,
      axis_builtin_extension_group_on_destroy_extensions);

  axis_env_on_create_instance_done(axis_env, ext_group, context, NULL);
}

void axis_builtin_extension_group_addon_destroy_instance(
    axis_UNUSED axis_addon_t *addon, axis_env_t *axis_env, void *_extension_group,
    void *context) {
  axis_extension_group_t *extension_group =
      (axis_extension_group_t *)_extension_group;
  axis_ASSERT(extension_group, "Invalid argument.");

  axis_extension_group_destroy(extension_group);

  axis_env_on_destroy_instance_done(axis_env, context, NULL);
}

static axis_addon_t builtin_extension_group_addon = {
    NULL,
    axis_ADDON_SIGNATURE,
    axis_builtin_extension_group_addon_on_init,
    NULL,
    axis_builtin_extension_group_addon_create_instance,
    axis_builtin_extension_group_addon_destroy_instance,
    NULL,
    NULL,
};

void axis_builtin_extension_group_addon_register(void) {
  axis_addon_register_extension_group(axis_STR_DEFAULT_EXTENSION_GROUP, NULL,
                                     &builtin_extension_group_addon, NULL);
}

void axis_builtin_extension_group_addon_unregister(void) {
  axis_addon_unregister_extension_group(axis_STR_DEFAULT_EXTENSION_GROUP);
}
