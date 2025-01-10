//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "axis_runtime/binding/go/interface/aptima/common.h"
#include "axis_runtime/binding/go/interface/aptima/axis_env.h"
#include "axis_runtime/common/errno.h"
#include "axis_runtime/axis_env/internal/metadata.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/event.h"
#include "axis_utils/lib/string.h"

typedef struct axis_notify_set_init_property_ctx_t {
  axis_string_t value;
  axis_error_t err;
  axis_event_t *completed;
} axis_env_notify_init_property_ctx_t;

static axis_env_notify_init_property_ctx_t *
axis_env_notify_init_property_ctx_create(const void *value, int value_len) {
  axis_env_notify_init_property_ctx_t *ctx =
      axis_MALLOC(sizeof(axis_env_notify_init_property_ctx_t));
  axis_ASSERT(ctx, "Failed to allocate memory.");

  axis_string_init_formatted(&ctx->value, "%.*s", value_len, value);
  axis_error_init(&ctx->err);
  ctx->completed = axis_event_create(0, 1);

  return ctx;
}

static void axis_env_notify_init_property_ctx_destroy(
    axis_env_notify_init_property_ctx_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->value);
  axis_error_deinit(&self->err);
  axis_event_destroy(self->completed);

  axis_FREE(self);
}

static void axis_env_proxy_notify_init_property_from_json(axis_env_t *axis_env,
                                                         void *user_data) {
  axis_ASSERT(user_data, "Invalid argument.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  axis_env_notify_init_property_ctx_t *ctx = user_data;
  axis_ASSERT(ctx, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  axis_env_init_property_from_json(axis_env, axis_string_get_raw_str(&ctx->value),
                                  &err);

  axis_event_set(ctx->completed);

  axis_error_deinit(&err);
}

axis_go_error_t axis_go_axis_env_init_property_from_json_bytes(
    uintptr_t bridge_addr, const void *json_str, int json_str_len) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self),
             "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(self, {
    axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_axis_IS_CLOSED);
    return cgo_error;
  });

  axis_env_notify_init_property_ctx_t *ctx =
      axis_env_notify_init_property_ctx_create(json_str, json_str_len);
  axis_ASSERT(ctx, "Should not happen.");

  if (!axis_env_proxy_notify(self->c_axis_env_proxy,
                            axis_env_proxy_notify_init_property_from_json, ctx,
                            false, &ctx->err)) {
    goto done;
  }

  axis_event_wait(ctx->completed, -1);

done:
  axis_go_error_from_error(&cgo_error, &ctx->err);
  axis_env_notify_init_property_ctx_destroy(ctx);
  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return cgo_error;
}
