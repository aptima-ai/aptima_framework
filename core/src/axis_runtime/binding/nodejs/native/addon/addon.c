//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/addon/addon.h"

#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/binding/nodejs/addon/addon_manager.h"
#include "include_internal/axis_runtime/binding/nodejs/common/common.h"
#include "include_internal/axis_runtime/binding/nodejs/common/tsfn.h"
#include "include_internal/axis_runtime/binding/nodejs/extension/extension.h"
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"
#include "js_native_api.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/sanitizer/thread_check.h"

typedef struct addon_on_xxx_callback_info_t {
  axis_nodejs_addon_t *addon_bridge;
  axis_env_t *axis_env;
} addon_on_xxx_callback_info_t;

typedef struct addon_on_create_instance_callback_ctx_t {
  axis_nodejs_addon_t *addon_bridge;
  axis_env_t *axis_env;
  axis_string_t instance_name;
  void *context;
} addon_on_create_instance_callback_ctx_t;

static addon_on_create_instance_callback_ctx_t *
addon_on_create_instance_callback_ctx_create(axis_nodejs_addon_t *addon_bridge,
                                             axis_env_t *axis_env,
                                             const char *instance_name,
                                             void *context) {
  addon_on_create_instance_callback_ctx_t *ctx =
      axis_MALLOC(sizeof(addon_on_create_instance_callback_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  ctx->addon_bridge = addon_bridge;
  ctx->axis_env = axis_env;
  axis_string_init_from_c_str(&ctx->instance_name, instance_name,
                             strlen(instance_name));
  ctx->context = context;

  return ctx;
}

static void addon_on_create_instance_callback_ctx_destroy(
    addon_on_create_instance_callback_ctx_t *ctx) {
  axis_ASSERT(ctx, "Should not happen.");

  axis_string_deinit(&ctx->instance_name);
  axis_FREE(ctx);
}

bool axis_nodejs_addon_check_integrity(axis_nodejs_addon_t *self,
                                      bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_NODEJS_ADDON_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

static void axis_nodejs_addon_detach_callbacks(axis_nodejs_addon_t *self) {
  axis_nodejs_tsfn_dec_rc(self->js_on_init);
  axis_nodejs_tsfn_dec_rc(self->js_on_deinit);
  axis_nodejs_tsfn_dec_rc(self->js_on_create_instance);
}

static void axis_nodejs_addon_destroy(axis_nodejs_addon_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_string_deinit(&self->addon_name);
  axis_sanitizer_thread_check_deinit(&self->thread_check);
  axis_nodejs_addon_detach_callbacks(self);

  axis_FREE(self);
}

static void axis_nodejs_addon_finalize(napi_env env, void *data, void *hint) {
  axis_LOGI("APTIMA JS Addon is finalized.");

  axis_nodejs_addon_t *addon_bridge = data;
  axis_ASSERT(
      addon_bridge && axis_nodejs_addon_check_integrity(addon_bridge, true),
      "Should not happen.");

  napi_status status = napi_ok;

  status = napi_delete_reference(env, addon_bridge->bridge.js_instance_ref);
  axis_ASSERT(status == napi_ok, "Failed to delete JS addon reference: %d",
             status);

  addon_bridge->bridge.js_instance_ref = NULL;

  axis_nodejs_addon_destroy(addon_bridge);
}

void axis_nodejs_invoke_addon_js_on_init(napi_env env, napi_value fn,
                                        void *context, void *data) {
  addon_on_xxx_callback_info_t *call_info = data;
  axis_ASSERT(call_info, "Should not happen.");

  axis_ASSERT(call_info->addon_bridge && axis_nodejs_addon_check_integrity(
                                            call_info->addon_bridge, true),
             "Should not happen.");

  axis_nodejs_axis_env_t *axis_env_bridge = NULL;
  napi_value js_axis_env = axis_nodejs_axis_env_create_new_js_object_and_wrap(
      env, call_info->axis_env, &axis_env_bridge);
  axis_ASSERT(js_axis_env, "Should not happen.");

  // Increase the reference count of the JS axis_env object to prevent it from
  // being garbage collected.
  uint32_t js_axis_env_ref_count = 0;
  napi_reference_ref(env, axis_env_bridge->bridge.js_instance_ref,
                     &js_axis_env_ref_count);

  napi_status status = napi_ok;

  {
    // Call on_init() of the APTIMA JS addon.

    // Get the APTIMA JS addon.
    napi_value js_addon = NULL;
    status = napi_get_reference_value(
        env, call_info->addon_bridge->bridge.js_instance_ref, &js_addon);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok && js_addon != NULL,
                            "Failed to get JS addon: %d", status);

    // Call on_init().
    napi_value result = NULL;
    napi_value argv[] = {js_axis_env};
    status = napi_call_function(env, js_addon, fn, 1, argv, &result);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok,
                            "Failed to call JS addon on_init(): %d", status);
  }

  goto done;

error:
  axis_LOGE("Failed to call JS addon on_init().");

done:
  axis_FREE(call_info);
}

void axis_nodejs_invoke_addon_js_on_deinit(napi_env env, napi_value fn,
                                          void *context, void *data) {
  addon_on_xxx_callback_info_t *call_info = data;
  axis_ASSERT(call_info, "Should not happen.");

  axis_ASSERT(call_info->addon_bridge && axis_nodejs_addon_check_integrity(
                                            call_info->addon_bridge, true),
             "Should not happen.");

  axis_nodejs_axis_env_t *axis_env_bridge =
      axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)call_info->axis_env);
  axis_ASSERT(axis_env_bridge &&
                 axis_nodejs_axis_env_check_integrity(axis_env_bridge, true),
             "Should not happen.");

  napi_status status = napi_ok;

  {
    // Call on_deinit() of the APTIMA JS addon.

    // Get the APTIMA JS addon.
    napi_value js_addon = NULL;
    status = napi_get_reference_value(
        env, call_info->addon_bridge->bridge.js_instance_ref, &js_addon);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok && js_addon != NULL,
                            "Failed to get JS addon: %d", status);

    napi_value js_axis_env = NULL;
    status = napi_get_reference_value(
        env, axis_env_bridge->bridge.js_instance_ref, &js_axis_env);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok && js_axis_env != NULL,
                            "Failed to get JS axis_env: %d", status);

    // Call on_deinit().
    napi_value result = NULL;
    napi_value argv[] = {js_axis_env};
    status = napi_call_function(env, js_addon, fn, 1, argv, &result);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok,
                            "Failed to call JS addon on_deinit(): %d", status);
  }

  goto done;

error:
  axis_LOGE("Failed to call JS addon on_deinit().");

done:
  axis_FREE(call_info);
}

void axis_nodejs_invoke_addon_js_on_create_instance(napi_env env, napi_value fn,
                                                   void *context, void *data) {
  addon_on_create_instance_callback_ctx_t *call_info = data;
  axis_ASSERT(call_info, "Should not happen.");

  axis_ASSERT(call_info->addon_bridge && axis_nodejs_addon_check_integrity(
                                            call_info->addon_bridge, true),
             "Should not happen.");

  axis_nodejs_axis_env_t *axis_env_bridge =
      axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)call_info->axis_env);
  axis_ASSERT(axis_env_bridge &&
                 axis_nodejs_axis_env_check_integrity(axis_env_bridge, true),
             "Should not happen.");

  napi_status status = napi_ok;

  {
    // Call on_create_instance() of the APTIMA JS addon.

    // Get the APTIMA JS addon.
    napi_value js_addon = NULL;
    status = napi_get_reference_value(
        env, call_info->addon_bridge->bridge.js_instance_ref, &js_addon);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok && js_addon != NULL,
                            "Failed to get JS addon: %d", status);

    napi_value js_axis_env = NULL;
    status = napi_get_reference_value(
        env, axis_env_bridge->bridge.js_instance_ref, &js_axis_env);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok && js_axis_env != NULL,
                            "Failed to get JS axis_env: %d", status);

    napi_value js_instance_name = NULL;
    status = napi_create_string_utf8(
        env, axis_string_get_raw_str(&call_info->instance_name),
        axis_string_len(&call_info->instance_name), &js_instance_name);
    GOTO_LABEL_IF_NAPI_FAIL(error,
                            status == napi_ok && js_instance_name != NULL,
                            "Failed to create JS instance name: %d", status);

    napi_value js_context = NULL;
    status =
        napi_create_external(env, call_info->context, NULL, NULL, &js_context);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok && js_context != NULL,
                            "Failed to create JS context: %d", status);

    napi_value result = NULL;
    napi_value argv[] = {js_axis_env, js_instance_name, js_context};
    status = napi_call_function(env, js_addon, fn, 3, argv, &result);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok,
                            "Failed to call JS addon on_create_instance(): %d",
                            status);
  }

  goto done;

error:
  axis_LOGE("Failed to call JS addon on_create_instance().");

done:
  addon_on_create_instance_callback_ctx_destroy(call_info);
}

static void proxy_on_init(axis_addon_t *addon, axis_env_t *axis_env) {
  axis_LOGI("addon proxy_on_init");

  axis_nodejs_addon_t *addon_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)addon);
  axis_ASSERT(addon_bridge &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: this function is called from a standalone
                 // addon registration thread, and only when all the addons are
                 // registered, the RTE world can go on, so it's thread safe.
                 axis_nodejs_addon_check_integrity(addon_bridge, false),
             "Should not happen.");

  addon_on_xxx_callback_info_t *call_info =
      axis_MALLOC(sizeof(addon_on_xxx_callback_info_t));
  axis_ASSERT(call_info, "Failed to allocate memory.");

  call_info->addon_bridge = addon_bridge;
  call_info->axis_env = axis_env;

  bool rc = axis_nodejs_tsfn_invoke(addon_bridge->js_on_init, call_info);
  if (!rc) {
    axis_LOGE("Failed to call addon on_init().");
    axis_FREE(call_info);

    // Failed to call JS on_init(), so that we need to call on_init_done() here
    // to let RTE runtime proceed.
    axis_env_on_init_done(axis_env, NULL);
  }
}

static void proxy_on_deinit(axis_addon_t *addon, axis_env_t *axis_env) {
  axis_LOGI("addon proxy_on_deinit");

  axis_nodejs_addon_t *addon_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)addon);
  axis_ASSERT(addon_bridge &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: this function is called from a standalone
                 // addon registration thread, and only when all the addons are
                 // registered, the RTE world can go on, so it's thread safe.
                 axis_nodejs_addon_check_integrity(addon_bridge, false),
             "Should not happen.");

  addon_on_xxx_callback_info_t *call_info =
      axis_MALLOC(sizeof(addon_on_xxx_callback_info_t));
  axis_ASSERT(call_info, "Failed to allocate memory.");

  call_info->addon_bridge = addon_bridge;
  call_info->axis_env = axis_env;

  bool rc = axis_nodejs_tsfn_invoke(addon_bridge->js_on_deinit, call_info);
  if (!rc) {
    axis_LOGE("Failed to call addon on_deinit().");
    axis_FREE(call_info);

    // Failed to call JS on_deinit(), so that we need to call on_deinit_done()
    // here to let RTE runtime proceed.
    axis_env_on_deinit_done(axis_env, NULL);
  }
}

static void proxy_on_create_instance(axis_addon_t *addon, axis_env_t *axis_env,
                                     const char *name, void *context) {
  axis_LOGI("addon proxy_on_create_instance name: %s", name);

  axis_nodejs_addon_t *addon_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)addon);
  axis_ASSERT(addon_bridge &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: this function is called from a standalone
                 // addon registration thread, and only when all the addons are
                 // registered, the RTE world can go on, so it's thread safe.
                 axis_nodejs_addon_check_integrity(addon_bridge, false),
             "Should not happen.");

  addon_on_create_instance_callback_ctx_t *call_info =
      addon_on_create_instance_callback_ctx_create(addon_bridge, axis_env, name,
                                                   context);

  bool rc =
      axis_nodejs_tsfn_invoke(addon_bridge->js_on_create_instance, call_info);
  axis_ASSERT(rc, "Failed to call addon on_create_instance().");
}

static void proxy_on_destroy_instance(axis_addon_t *addon, axis_env_t *axis_env,
                                      void *instance, void *context) {
  axis_LOGI("addon proxy_on_destroy_instance");

  axis_nodejs_addon_t *addon_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)addon);
  axis_ASSERT(addon_bridge &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 axis_nodejs_addon_check_integrity(addon_bridge, false),
             "Should not happen.");

  axis_ASSERT(addon_bridge->c_addon_host->type == axis_ADDON_TYPE_EXTENSION,
             "Should not happen.");

  axis_nodejs_extension_t *extension_bridge =
      axis_binding_handle_get_me_in_target_lang(
          (axis_binding_handle_t *)instance);
  axis_ASSERT(extension_bridge &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 axis_nodejs_extension_check_integrity(extension_bridge, false),
             "Should not happen.");

  axis_env_on_destroy_instance_done(axis_env, context, NULL);
}

static napi_value axis_nodejs_addon_create(napi_env env,
                                          napi_callback_info info) {
  axis_ASSERT(env, "Should not happen.");

  const size_t argc = 1;
  napi_value args[argc];  // this
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    goto done;
  }

  axis_nodejs_addon_t *addon_bridge = axis_MALLOC(sizeof(axis_nodejs_addon_t));
  axis_ASSERT(addon_bridge, "Failed to allocate memory for addon bridge.");

  axis_signature_set(&addon_bridge->signature, axis_NODEJS_ADDON_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(
      &addon_bridge->thread_check);

  axis_string_init(&addon_bridge->addon_name);
  addon_bridge->c_addon_host = NULL;

  napi_status status =
      napi_wrap(env, args[0], addon_bridge, axis_nodejs_addon_finalize, NULL,
                &addon_bridge->bridge.js_instance_ref);
  GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok,
                          "Failed to bind JS addon & bridge: %d", status);

  // Increase the reference count of the JS addon object.
  uint32_t js_addon_ref_count = 0;
  status = napi_reference_ref(env, addon_bridge->bridge.js_instance_ref,
                              &js_addon_ref_count);
  GOTO_LABEL_IF_NAPI_FAIL(
      error, status == napi_ok,
      "Failed to increase the reference count of JS addon: %d", status);

  // Create the underlying APTIMA C addon.
  axis_addon_init(&addon_bridge->c_addon, proxy_on_init, proxy_on_deinit,
                 proxy_on_create_instance, proxy_on_destroy_instance, NULL);

  axis_binding_handle_set_me_in_target_lang(
      (axis_binding_handle_t *)&addon_bridge->c_addon, addon_bridge);

  goto done;

error:
  if (addon_bridge) {
    axis_FREE(addon_bridge);
  }

done:
  return js_undefined(env);
}

static void axis_nodejs_addon_release_js_on_xxx_tsfn(
    napi_env env, axis_nodejs_addon_t *addon_bridge) {
  axis_ASSERT(env && addon_bridge, "Should not happen.");

  axis_nodejs_tsfn_release(addon_bridge->js_on_init);
  axis_nodejs_tsfn_release(addon_bridge->js_on_deinit);
  axis_nodejs_tsfn_release(addon_bridge->js_on_create_instance);
}

static napi_value axis_nodejs_addon_on_end_of_life(napi_env env,
                                                  napi_callback_info info) {
  axis_ASSERT(env && info, "Should not happen.");

  const size_t argc = 1;
  napi_value args[argc];  // this
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_addon_t *addon_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&addon_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && addon_bridge != NULL,
                                "Failed to get addon bridge: %d", status);
  axis_ASSERT(
      addon_bridge && axis_nodejs_addon_check_integrity(addon_bridge, true),
      "Should not happen.");

  // From now on, the JS on_xxx callback(s) are useless, so release them all.
  axis_nodejs_addon_release_js_on_xxx_tsfn(env, addon_bridge);

  // Decrease the reference count of the JS addon object.
  uint32_t js_addon_ref_count = 0;
  status = napi_reference_unref(env, addon_bridge->bridge.js_instance_ref,
                                &js_addon_ref_count);

  return js_undefined(env);
}

napi_value axis_nodejs_addon_module_init(napi_env env, napi_value exports) {
  axis_ASSERT(env && exports, "Should not happen.");

  EXPORT_FUNC(env, exports, axis_nodejs_addon_create);
  EXPORT_FUNC(env, exports,
              axis_nodejs_addon_manager_register_addon_as_extension);
  EXPORT_FUNC(env, exports, axis_nodejs_addon_on_end_of_life);

  return exports;
}
