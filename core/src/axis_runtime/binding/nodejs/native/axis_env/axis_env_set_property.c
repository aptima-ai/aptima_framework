//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/axis_env/axis_env.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/memory.h"

typedef struct axis_env_notify_set_property_ctx_t {
  axis_string_t path;
  axis_value_t *value;
  axis_nodejs_tsfn_t *js_cb;
} axis_env_notify_set_property_ctx_t;

static axis_env_notify_set_property_ctx_t *
axis_env_notify_set_property_ctx_create(const void *path, axis_value_t *value,
                                       axis_nodejs_tsfn_t *js_cb) {
  axis_env_notify_set_property_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_set_property_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  axis_string_init_from_c_str(&ctx->path, path, strlen(path));
  ctx->value = value;
  ctx->js_cb = js_cb;

  return ctx;
}

static void axis_env_notify_set_property_ctx_destroy(
    axis_env_notify_set_property_ctx_t *ctx) {
  axis_ASSERT(ctx, "Invalid argument.");

  axis_string_deinit(&ctx->path);

  if (ctx->value) {
    axis_value_destroy(ctx->value);
  }

  axis_FREE(ctx);
}

static void axis_env_proxy_notify_set_property(axis_env_t *axis_env,
                                              void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_set_property_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_nodejs_tsfn_t *js_cb = ctx->js_cb;
  axis_ASSERT(js_cb && axis_nodejs_tsfn_check_integrity(js_cb, false),
             "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_env_set_property(axis_env, axis_string_get_raw_str(&ctx->path),
                                 ctx->value, &err);

  axis_error_t *cloned_error = NULL;
  if (!rc) {
    cloned_error = axis_error_create();
    axis_error_copy(cloned_error, &err);
  }

  axis_nodejs_set_property_call_ctx_t *call_info =
      axis_nodejs_set_property_call_ctx_create(js_cb, rc, cloned_error);
  axis_ASSERT(call_info, "Should not happen.");

  bool call_js_rc = axis_nodejs_tsfn_invoke(js_cb, call_info);
  axis_ASSERT(call_js_rc, "Should not happen.");

  if (rc) {
    // The ownership of the value is transferred to the runtime.
    ctx->value = NULL;
  }

  axis_env_notify_set_property_ctx_destroy(ctx);

  axis_error_deinit(&err);
}

bool axis_nodejs_axis_env_set_property_value(axis_nodejs_axis_env_t *self,
                                           const char *path, axis_value_t *value,
                                           axis_nodejs_tsfn_t *cb_tsfn,
                                           axis_error_t *error) {
  axis_ASSERT(self && axis_nodejs_axis_env_check_integrity(self, true),
             "Invalid argument.");
  axis_ASSERT(path, "Invalid argument.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Invalid argument.");
  axis_ASSERT(cb_tsfn && axis_nodejs_tsfn_check_integrity(cb_tsfn, true),
             "Invalid argument.");

  axis_env_notify_set_property_ctx_t *ctx =
      axis_env_notify_set_property_ctx_create(path, value, cb_tsfn);
  axis_ASSERT(ctx, "Should not happen.");

  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_set_property, ctx, false,
                            error)) {
    axis_env_notify_set_property_ctx_destroy(ctx);
    return false;
  }

  return true;
}
