//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/axis_env/metadata.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/schema_store/store.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value_object.h"

bool axis_app_set_property(axis_app_t *app, const char *name, axis_value_t *value,
                          axis_error_t *err) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Invalid argument.");
  axis_ASSERT(name && value && axis_value_check_integrity(value),
             "Invalid argument.");

  if (!axis_schema_store_adjust_property_kv(&app->schema_store, name, value,
                                           err)) {
    return false;
  }

  if (!axis_schema_store_validate_property_kv(&app->schema_store, name, value,
                                             err)) {
    return false;
  }

  return axis_value_object_move(&app->property, name, value);
}

static axis_app_set_property_context_t *set_property_context_create(
    const char *name, axis_value_t *value, axis_app_set_property_async_cb_t cb,
    void *cb_data) {
  axis_app_set_property_context_t *set_prop =
      axis_MALLOC(sizeof(axis_app_set_property_context_t));
  axis_ASSERT(set_prop, "Failed to allocate memory.");

  axis_string_init_formatted(&set_prop->name, name);
  set_prop->value = value;
  set_prop->cb = cb;
  set_prop->cb_data = cb_data;
  axis_error_init(&set_prop->err);

  return set_prop;
}

static void set_property_context_destroy(axis_app_set_property_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_error_deinit(&self->err);
  axis_string_deinit(&self->name);
  axis_FREE(self);
}

static void axis_app_set_property_task(void *self_, void *arg) {
  axis_app_t *self = self_;
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  axis_app_set_property_context_t *set_property_context = arg;
  axis_ASSERT(set_property_context, "Should not happen.");

  axis_app_set_property(self,
                       axis_string_get_raw_str(&set_property_context->name),
                       set_property_context->value, &set_property_context->err);

  if (set_property_context->cb) {
    set_property_context->cb(self, &set_property_context->err,
                             set_property_context->cb_data);
  }

  set_property_context_destroy(set_property_context);
}

void axis_app_set_property_async(axis_app_t *self, const char *name,
                                axis_value_t *value,
                                axis_app_set_property_async_cb_t cb,
                                void *cb_data) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_app_check_integrity(self, false), "Invalid use of app %p.",
             self);

  axis_app_set_property_context_t *set_property_context =
      set_property_context_create(name, value, cb, cb_data);

  axis_runloop_post_task_tail(axis_app_get_attached_runloop(self),
                             axis_app_set_property_task, self,
                             set_property_context);
}

axis_value_t *axis_app_peek_property(axis_app_t *app, const char *name) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Invalid argument.");
  axis_ASSERT(name, "Invalid argument.");

  axis_value_t *item = axis_value_object_peek(&app->property, name);
  if (item == NULL) {
    return NULL;
  }

  return item;
}

static axis_app_peek_property_context_t *axis_app_peek_property_context_create(
    const char *name, axis_app_peek_property_async_cb_t cb, void *cb_data) {
  axis_app_peek_property_context_t *context =
      axis_MALLOC(sizeof(axis_app_peek_property_context_t));
  axis_ASSERT(context, "Failed to allocate memory.");

  axis_string_init_formatted(&context->name, name);
  context->cb = cb;
  context->cb_data = cb_data;
  context->res = NULL;

  return context;
}

static void axis_app_peek_property_context_destroy(
    axis_app_peek_property_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->name);

  axis_FREE(self);
}

static void axis_app_peek_property_task(void *self_, void *arg) {
  axis_app_t *self = (axis_app_t *)self_;
  axis_ASSERT(self && axis_app_check_integrity(self, true) && self->loop,
             "Should not happen.");

  axis_app_peek_property_context_t *context =
      (axis_app_peek_property_context_t *)arg;
  axis_ASSERT(context, "Should not happen.");

  context->res =
      axis_app_peek_property(self, axis_string_get_raw_str(&context->name));

  if (context->cb) {
    context->cb(self, context->res, context->cb_data);
  }

  axis_app_peek_property_context_destroy(context);
}

void axis_app_peek_property_async(axis_app_t *self, const char *name,
                                 axis_app_peek_property_async_cb_t cb,
                                 void *cb_data) {
  axis_ASSERT(
      self && axis_app_check_integrity(
                  self,
                  // axis_NOLINTNEXTLINE(thread-check)
                  // thread-check: This function maybe called from any thread.
                  false),
      "Invalid argument.");

  axis_app_peek_property_context_t *context =
      axis_app_peek_property_context_create(name, cb, cb_data);

  axis_runloop_post_task_tail(self->loop, axis_app_peek_property_task, self,
                             context);
}

axis_value_t *axis_app_peek_manifest(axis_app_t *self, const char *name) {
  axis_ASSERT(
      self && axis_app_check_integrity(
                  self,
                  // axis_NOLINTNEXTLINE(thread-check)
                  // thread-check: This function maybe called from any thread.
                  false),
      "Invalid argument.");

  axis_ASSERT(name, "Invalid argument.");

  axis_value_t *item = axis_value_object_peek(&self->manifest, name);
  if (item == NULL) {
    return NULL;
  }

  return item;
}

static axis_app_peek_manifest_context_t *axis_app_peek_manifest_context_create(
    const char *name, axis_app_peek_manifest_async_cb_t cb, void *cb_data) {
  axis_app_peek_manifest_context_t *context =
      axis_MALLOC(sizeof(axis_app_peek_manifest_context_t));
  axis_ASSERT(context, "Failed to allocate memory.");

  axis_string_init_formatted(&context->name, name);
  context->cb = cb;
  context->cb_data = cb_data;
  context->res = NULL;

  return context;
}

static void axis_app_peek_manifest_context_destroy(
    axis_app_peek_manifest_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->name);

  axis_FREE(self);
}

static void axis_app_peek_manifest_task(void *self_, void *arg) {
  axis_app_t *self = (axis_app_t *)self_;
  axis_ASSERT(self && axis_app_check_integrity(self, true) && self->loop,
             "Should not happen.");

  axis_app_peek_manifest_context_t *context =
      (axis_app_peek_manifest_context_t *)arg;
  axis_ASSERT(context, "Should not happen.");

  context->res =
      axis_app_peek_manifest(self, axis_string_get_raw_str(&context->name));

  if (context->cb) {
    context->cb(self, context->res, context->cb_data);
  }

  axis_app_peek_manifest_context_destroy(context);
}

void axis_app_peek_manifest_async(axis_app_t *self, const char *name,
                                 axis_app_peek_manifest_async_cb_t cb,
                                 void *cb_data) {
  axis_ASSERT(
      self && axis_app_check_integrity(
                  self,
                  // axis_NOLINTNEXTLINE(thread-check)
                  // thread-check: This function maybe called from any thread.
                  false),
      "Invalid argument.");

  axis_app_peek_manifest_context_t *context =
      axis_app_peek_manifest_context_create(name, cb, cb_data);

  axis_runloop_post_task_tail(self->loop, axis_app_peek_manifest_task, self,
                             context);
}
