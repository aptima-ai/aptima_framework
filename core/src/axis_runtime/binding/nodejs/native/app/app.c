//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/app/app.h"

#include "include_internal/axis_runtime/binding/nodejs/common/common.h"
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"
#include "js_native_api.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/axis_env/internal/on_xxx_done.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/sanitizer/thread_check.h"

typedef struct app_async_run_data_t {
  axis_nodejs_app_t *app_bridge;
  napi_deferred deferred;
  napi_async_work work;
  int async_action_status;
} app_async_run_data_t;

typedef struct app_on_xxx_call_info_t {
  axis_nodejs_app_t *app_bridge;
  axis_nodejs_axis_env_t *axis_env_bridge;
  axis_env_t *axis_env;
  axis_env_proxy_t *axis_env_proxy;
} app_on_xxx_call_info_t;

static bool axis_nodejs_app_check_integrity(axis_nodejs_app_t *self,
                                           bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_NODEJS_APP_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

static void invoke_app_js_on_configure(napi_env env, napi_value fn,
                                       axis_UNUSED void *context, void *data) {
  app_on_xxx_call_info_t *call_info = data;
  axis_ASSERT(call_info, "Should not happen.");

  axis_ASSERT(call_info->app_bridge &&
                 axis_nodejs_app_check_integrity(call_info->app_bridge, true),
             "Should not happen.");

  // Export the C axis_env object to the JS world.
  axis_nodejs_axis_env_t *axis_env_bridge = NULL;
  napi_value js_axis_env = axis_nodejs_axis_env_create_new_js_object_and_wrap(
      env, call_info->axis_env, &axis_env_bridge);
  axis_ASSERT(js_axis_env, "Should not happen.");

  axis_env_bridge->c_axis_env_proxy = call_info->axis_env_proxy;
  axis_ASSERT(axis_env_bridge->c_axis_env_proxy, "Should not happen.");

  // Increase the reference count of the JS axis_env object to prevent it from
  // being garbage collected.
  uint32_t js_axis_env_ref_count = 0;
  napi_reference_ref(env, axis_env_bridge->bridge.js_instance_ref,
                     &js_axis_env_ref_count);

  napi_status status = napi_ok;

  {
    // Call on_configure() of the APTIMA JS app.

    // Get the APTIMA JS app.
    napi_value js_app = NULL;
    status = napi_get_reference_value(
        env, call_info->app_bridge->bridge.js_instance_ref, &js_app);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok && js_app != NULL,
                            "Failed to get JS app: %d", status);

    // Call on_configure().
    napi_value result = NULL;
    napi_value argv[] = {js_axis_env};
    status = napi_call_function(env, js_app, fn, 1, argv, &result);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok,
                            "Failed to call JS app on_configure(): %d", status);
  }

  goto done;

error:
  axis_LOGE("Failed to call JS app on_configure().");

done:
  axis_FREE(call_info);
}

static void invoke_app_js_on_init(napi_env env, napi_value fn,
                                  axis_UNUSED void *context, void *data) {
  app_on_xxx_call_info_t *call_info = data;
  axis_ASSERT(call_info, "Should not happen.");

  axis_nodejs_app_t *app_bridge = call_info->app_bridge;
  axis_ASSERT(app_bridge && axis_nodejs_app_check_integrity(app_bridge, true),
             "Should not happen.");

  axis_nodejs_axis_env_t *axis_env_bridge = call_info->axis_env_bridge;
  axis_ASSERT(axis_env_bridge &&
                 axis_nodejs_axis_env_check_integrity(axis_env_bridge, true),
             "Should not happen.");

  napi_status status = napi_ok;

  {
    // Call on_init() of the APTIMA JS app.

    napi_value js_app = NULL;
    status = napi_get_reference_value(env, app_bridge->bridge.js_instance_ref,
                                      &js_app);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok && js_app != NULL,
                            "Failed to get JS app: %d", status);

    napi_value js_axis_env = NULL;
    status = napi_get_reference_value(
        env, axis_env_bridge->bridge.js_instance_ref, &js_axis_env);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok && js_axis_env != NULL,
                            "Failed to get JS axis_env: %d", status);

    napi_value result = NULL;
    napi_value argv[] = {js_axis_env};
    status = napi_call_function(env, js_app, fn, 1, argv, &result);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok,
                            "Failed to call JS app on_init(): %d", status);
  }

  goto done;

error:
  axis_LOGE("Failed to call JS app on_init().");

done:
  axis_FREE(call_info);
}

static void invoke_app_js_on_deinit(napi_env env, napi_value fn,
                                    axis_UNUSED void *context, void *data) {
  app_on_xxx_call_info_t *call_info = data;
  axis_ASSERT(call_info, "Should not happen.");

  axis_nodejs_app_t *app_bridge = call_info->app_bridge;
  axis_ASSERT(app_bridge && axis_nodejs_app_check_integrity(app_bridge, true),
             "Should not happen.");

  axis_nodejs_axis_env_t *axis_env_bridge = call_info->axis_env_bridge;
  axis_ASSERT(axis_env_bridge &&
                 axis_nodejs_axis_env_check_integrity(axis_env_bridge, true),
             "Should not happen.");

  napi_status status = napi_ok;

  {
    // Call on_deinit() of the APTIMA JS app.

    napi_value js_app = NULL;
    status = napi_get_reference_value(env, app_bridge->bridge.js_instance_ref,
                                      &js_app);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok && js_app != NULL,
                            "Failed to get JS app: %d", status);

    napi_value js_axis_env = NULL;
    status = napi_get_reference_value(
        env, axis_env_bridge->bridge.js_instance_ref, &js_axis_env);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok && js_axis_env != NULL,
                            "Failed to get JS axis_env: %d", status);

    napi_value result = NULL;
    napi_value argv[] = {js_axis_env};
    status = napi_call_function(env, js_app, fn, 1, argv, &result);
    GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok,
                            "Failed to call JS app on_deinit(): %d", status);
  }

  goto done;

error:
  axis_LOGE("Failed to call JS app on_deinit().");
  axis_ASSERT(0, "Should not happen.");

done:
  axis_FREE(call_info);
}

static void proxy_on_configure(axis_app_t *app, axis_env_t *axis_env) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_nodejs_app_t *app_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)app);
  axis_ASSERT(app_bridge &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: The ownership of the app_bridge is the JS main
                 // thread, therefore, in order to maintain thread safety, we
                 // use semaphore below to prevent JS main thread and the APTIMA
                 // app thread access the app bridge at the same time.
                 axis_nodejs_app_check_integrity(app_bridge, false),
             "Should not happen.");

  app_on_xxx_call_info_t *call_info =
      axis_MALLOC(sizeof(app_on_xxx_call_info_t));
  axis_ASSERT(call_info, "Failed to allocate memory.");
  call_info->app_bridge = app_bridge;
  call_info->axis_env = axis_env;
  call_info->axis_env_proxy = axis_env_proxy_create(axis_env, 1, NULL);

  bool rc = axis_nodejs_tsfn_invoke(app_bridge->js_on_configure, call_info);
  if (!rc) {
    axis_LOGE("Failed to call app on_init().");
    axis_FREE(call_info);

    // Failed to call JS on_init(), so that we need to call on_init_done() here
    // to let RTE runtime proceed.
    axis_env_on_configure_done(axis_env, NULL);
  }
}

static void proxy_on_init(axis_app_t *app, axis_env_t *axis_env) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_nodejs_app_t *app_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)app);
  axis_ASSERT(app_bridge &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 axis_nodejs_app_check_integrity(app_bridge, false),
             "Should not happen.");

  axis_nodejs_axis_env_t *axis_env_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)axis_env);
  axis_ASSERT(axis_env_bridge &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 axis_nodejs_axis_env_check_integrity(axis_env_bridge, false),
             "Should not happen.");

  app_on_xxx_call_info_t *call_info =
      axis_MALLOC(sizeof(app_on_xxx_call_info_t));
  axis_ASSERT(call_info, "Failed to allocate memory.");

  call_info->app_bridge = app_bridge;
  call_info->axis_env_bridge = axis_env_bridge;

  bool rc = axis_nodejs_tsfn_invoke(app_bridge->js_on_init, call_info);
  if (!rc) {
    axis_LOGE("Failed to call app on_init().");
    axis_FREE(call_info);

    // Failed to call JS on_init(), so that we need to call on_init_done() here
    // to let RTE runtime proceed.
    axis_env_on_init_done(axis_env, NULL);
  }
}

static void proxy_on_deinit(axis_app_t *app, axis_env_t *axis_env) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_nodejs_app_t *app_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)app);
  axis_ASSERT(app_bridge &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 axis_nodejs_app_check_integrity(app_bridge, false),
             "Should not happen.");

  axis_nodejs_axis_env_t *axis_env_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)axis_env);
  axis_ASSERT(axis_env_bridge &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 axis_nodejs_axis_env_check_integrity(axis_env_bridge, false),
             "Should not happen.");

  app_on_xxx_call_info_t *call_info =
      axis_MALLOC(sizeof(app_on_xxx_call_info_t));
  axis_ASSERT(call_info, "Failed to allocate memory.");

  call_info->app_bridge = app_bridge;
  call_info->axis_env_bridge = axis_env_bridge;

  bool rc = axis_nodejs_tsfn_invoke(app_bridge->js_on_deinit, call_info);
  axis_ASSERT(rc, "Failed to call app on_deinit().");
}

static void axis_nodejs_app_create_and_attach_callbacks(
    napi_env env, axis_nodejs_app_t *app_bridge) {
  axis_ASSERT(app_bridge && axis_nodejs_app_check_integrity(app_bridge, true),
             "Should not happen.");

  napi_value js_app = NULL;
  napi_status status = napi_get_reference_value(
      env, app_bridge->bridge.js_instance_ref, &js_app);
  ASSERT_IF_NAPI_FAIL(status == napi_ok && js_app != NULL,
                      "Failed to get JS app instance.");

  napi_value js_on_configure_proxy =
      axis_nodejs_get_property(env, js_app, "onConfigureProxy");
  CREATE_JS_CB_TSFN(app_bridge->js_on_configure, env, "[TSFN] app::onConfigure",
                    js_on_configure_proxy, invoke_app_js_on_configure);

  napi_value js_on_init_proxy =
      axis_nodejs_get_property(env, js_app, "onInitProxy");
  CREATE_JS_CB_TSFN(app_bridge->js_on_init, env, "[TSFN] app::onInit",
                    js_on_init_proxy, invoke_app_js_on_init);

  napi_value js_on_deinit_proxy =
      axis_nodejs_get_property(env, js_app, "onDeinitProxy");
  CREATE_JS_CB_TSFN(app_bridge->js_on_deinit, env, "[TSFN] app::onDeinit",
                    js_on_deinit_proxy, invoke_app_js_on_deinit);
}

static void axis_nodejs_app_release_js_on_xxx_tsfn(axis_nodejs_app_t *self) {
  axis_ASSERT(self && axis_nodejs_app_check_integrity(self, true),
             "Should not happen.");

  axis_nodejs_tsfn_release(self->js_on_configure);
  axis_nodejs_tsfn_release(self->js_on_init);
  axis_nodejs_tsfn_release(self->js_on_deinit);
}

static void axis_nodejs_app_detach_callbacks(axis_nodejs_app_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: if reach here, it means JS app & C app are all
                 // end, so we can not check the thread safety here.
                 axis_nodejs_app_check_integrity(self, false),
             "Should not happen.");

  /**
   * The app holds references to its TSFN, and it's time to drop the references.
   */
  axis_nodejs_tsfn_dec_rc(self->js_on_configure);
  axis_nodejs_tsfn_dec_rc(self->js_on_init);
  axis_nodejs_tsfn_dec_rc(self->js_on_deinit);
}

static void axis_nodejs_app_destroy(axis_nodejs_app_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: if reach here, it means JS app & C app are all
                 // end, so it's thread safe here.
                 axis_nodejs_app_check_integrity(self, false),
             "Should not happen.");
  axis_nodejs_app_detach_callbacks(self);
  axis_sanitizer_thread_check_deinit(&self->thread_check);
  axis_FREE(self);
}

// Invoked when the JS app finalizes.
static void axis_nodejs_app_finalize(napi_env env, void *data, void *hint) {
  axis_LOGI("APTIMA JS app is finalized.");

  axis_nodejs_app_t *app_bridge = data;
  axis_ASSERT(app_bridge && axis_nodejs_app_check_integrity(app_bridge, true),
             "Should not happen.");

  napi_status status = napi_ok;

  status = napi_delete_reference(env, app_bridge->bridge.js_instance_ref);
  axis_ASSERT(status == napi_ok, "Failed to delete JS app reference: %d",
             status);

  app_bridge->bridge.js_instance_ref = NULL;
  // Destroy the underlying APTIMA C app.
  axis_app_destroy(app_bridge->c_app);

  axis_nodejs_app_destroy(app_bridge);
}

static napi_value axis_nodejs_app_create(napi_env env, napi_callback_info info) {
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

  axis_nodejs_app_t *app_bridge = axis_MALLOC(sizeof(axis_nodejs_app_t));
  axis_ASSERT(app_bridge, "Failed to allocate memory.");

  axis_signature_set(&app_bridge->signature, axis_NODEJS_APP_SIGNATURE);

  // The ownership of the APTIMA app_bridge is the JS main thread.
  axis_sanitizer_thread_check_init_with_current_thread(
      &app_bridge->thread_check);

  // Wraps the native bridge instance ('app_bridge') in the javascript APP
  // object (args[0]). And the returned reference ('js_ref' here) is a weak
  // reference, meaning it has a reference count of 0.
  napi_status status =
      napi_wrap(env, args[0], app_bridge, axis_nodejs_app_finalize, NULL,
                &app_bridge->bridge.js_instance_ref);
  GOTO_LABEL_IF_NAPI_FAIL(error, status == napi_ok,
                          "Failed to bind JS app & bridge: %d", status);

  // Create the underlying APTIMA C app.
  app_bridge->c_app =
      axis_app_create(proxy_on_configure, proxy_on_init, proxy_on_deinit, NULL);
  axis_binding_handle_set_me_in_target_lang(
      (axis_binding_handle_t *)(app_bridge->c_app), app_bridge);

  goto done;

error:
  if (app_bridge) {
    axis_FREE(app_bridge);
  }

done:
  return js_undefined(env);
}

static void axis_nodejs_app_run_async_work(napi_env env, axis_UNUSED void *data) {
  axis_ASSERT(env, "Should not happen.");

  app_async_run_data_t *async_run_data = data;
  axis_ASSERT(async_run_data, "Should not happen.");

  // Run the APTIMA app in another thread, so that the APTIMA app thread won't block
  // the JS main thread.
  axis_app_run(async_run_data->app_bridge->c_app, false, NULL);

  async_run_data->async_action_status = 0;
}

static void axis_nodejs_app_run_async_work_complete(napi_env env,
                                                   napi_status status,
                                                   axis_UNUSED void *data) {
  axis_ASSERT(env, "Should not happen.");

  app_async_run_data_t *async_run_data = data;
  axis_ASSERT(async_run_data, "Should not happen.");

  if (async_run_data->async_action_status == 0) {
    // The APTIMA app has been run successfully.
    status =
        napi_resolve_deferred(env, async_run_data->deferred, js_undefined(env));
  } else {
    // The APTIMA app failed to run.
    status =
        napi_reject_deferred(env, async_run_data->deferred, js_undefined(env));
  }

  napi_delete_async_work(env, async_run_data->work);
  axis_FREE(async_run_data);
}

static napi_value axis_nodejs_app_run(napi_env env, napi_callback_info info) {
  axis_ASSERT(env && info, "Should not happen.");

  axis_LOGD("App run.");

  const size_t argc = 1;
  napi_value args[1];  // this
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_app_t *app_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&app_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && app_bridge != NULL,
                                "Failed to get app bridge: %d", status);
  axis_ASSERT(app_bridge && axis_nodejs_app_check_integrity(app_bridge, true),
             "Should not happen.");

  // Increase the reference count of the JS app object to prevent it from being
  // garbage collected. The reference count will be decreased when the app is
  // deinited.
  uint32_t js_app_ref_count = 0;
  status = napi_reference_ref(env, app_bridge->bridge.js_instance_ref,
                              &js_app_ref_count);

  // Create and attach callbacks which will be invoked during the runtime of the
  // APTIMA app.
  axis_nodejs_app_create_and_attach_callbacks(env, app_bridge);

  app_async_run_data_t *async_run_data =
      axis_MALLOC(sizeof(app_async_run_data_t));
  axis_ASSERT(async_run_data, "Failed to allocate memory.");

  async_run_data->app_bridge = app_bridge;
  async_run_data->async_action_status = 1;

  napi_value promise = NULL;
  status = napi_create_promise(env, &async_run_data->deferred, &promise);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && promise != NULL,
                                "Failed to create promise: %d", status);

  // Create an async work to run the APTIMA app in another thread.
  status = napi_create_async_work(env, NULL, js_undefined(env),
                                  axis_nodejs_app_run_async_work,
                                  axis_nodejs_app_run_async_work_complete,
                                  async_run_data, &async_run_data->work);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok,
                                "Failed to create async work: %d", status);

  status = napi_queue_async_work(env, async_run_data->work);

  return promise;
}

static napi_value axis_nodejs_app_close(napi_env env, napi_callback_info info) {
  axis_ASSERT(env && info, "Should not happen.");

  axis_LOGD("App close.");

  const size_t argc = 1;
  napi_value args[argc];  // this
  if (!axis_nodejs_get_js_func_args(env, info, args, argc)) {
    napi_fatal_error(NULL, NAPI_AUTO_LENGTH,
                     "Incorrect number of parameters passed.",
                     NAPI_AUTO_LENGTH);
    axis_ASSERT(0, "Should not happen.");
    return js_undefined(env);
  }

  axis_nodejs_app_t *app_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&app_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && app_bridge != NULL,
                                "Failed to get app bridge: %d", status);
  axis_ASSERT(app_bridge && axis_nodejs_app_check_integrity(app_bridge, true),
             "Should not happen.");

  axis_app_close(app_bridge->c_app, NULL);

  return js_undefined(env);
}

static napi_value axis_nodejs_app_on_end_of_life(napi_env env,
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

  axis_nodejs_app_t *app_bridge = NULL;
  napi_status status = napi_unwrap(env, args[0], (void **)&app_bridge);
  RETURN_UNDEFINED_IF_NAPI_FAIL(status == napi_ok && app_bridge != NULL,
                                "Failed to get app bridge: %d", status);
  axis_ASSERT(app_bridge && axis_nodejs_app_check_integrity(app_bridge, true),
             "Should not happen.");

  // From now on, the JS on_xxx callback(s) are useless, so release them all.
  axis_nodejs_app_release_js_on_xxx_tsfn(app_bridge);

  // Decrease the reference count of the JS app object.
  uint32_t js_app_ref_count = 0;
  status = napi_reference_unref(env, app_bridge->bridge.js_instance_ref,
                                &js_app_ref_count);
  axis_ASSERT(status == napi_ok, "Failed to unref JS app.");

  return js_undefined(env);
}

napi_value axis_nodejs_app_module_init(napi_env env, napi_value exports) {
  axis_ASSERT(env && exports, "Should not happen.");

  EXPORT_FUNC(env, exports, axis_nodejs_app_create);
  EXPORT_FUNC(env, exports, axis_nodejs_app_run);
  EXPORT_FUNC(env, exports, axis_nodejs_app_close);
  EXPORT_FUNC(env, exports, axis_nodejs_app_on_end_of_life);

  return exports;
}
