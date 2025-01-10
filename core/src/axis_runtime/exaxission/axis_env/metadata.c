//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/axis_env/metadata.h"

#include "include_internal/axis_runtime/app/axis_env/metadata.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/metadata.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/schema_store/store.h"
#include "include_internal/axis_runtime/axis_env/metadata_cb.h"
#include "include_internal/axis_utils/value/value_path.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value.h"

bool axis_extension_set_property(axis_extension_t *self, const char *name,
                                axis_value_t *value, axis_error_t *err) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Invalid argument.");

  if (!axis_schema_store_adjust_property_kv(&self->schema_store, name, value,
                                           err)) {
    return false;
  }

  if (!axis_schema_store_validate_property_kv(&self->schema_store, name, value,
                                             err)) {
    return false;
  }

  return axis_value_set_from_path_str_with_move(&self->property, name, value,
                                               err);
}

static axis_extension_set_property_context_t *set_property_context_create(
    const char *name, axis_value_t *value,
    axis_extension_set_property_async_cb_t cb, void *cb_data) {
  axis_extension_set_property_context_t *set_prop =
      axis_MALLOC(sizeof(axis_extension_set_property_context_t));
  axis_ASSERT(set_prop, "Failed to allocate memory.");

  axis_string_init_formatted(&set_prop->path, name);
  set_prop->value = value;
  set_prop->cb = cb;
  set_prop->cb_data = cb_data;
  set_prop->res = false;

  return set_prop;
}

static void set_property_context_destroy(
    axis_extension_set_property_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->path);
  axis_FREE(self);
}

static void axis_extension_set_property_task(void *self_, void *arg) {
  axis_extension_t *self = self_;
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  axis_extension_thread_t *extension_thread = self->extension_thread;
  axis_ASSERT(extension_thread, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(extension_thread, true),
             "Invalid use of extension_thread %p.", extension_thread);

  axis_extension_set_property_context_t *set_property_context = arg;
  axis_ASSERT(set_property_context, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  set_property_context->res = axis_extension_set_property(
      self, axis_string_get_raw_str(&set_property_context->path),
      set_property_context->value, &err);

  if (set_property_context->cb) {
    set_property_context->cb(self, set_property_context->res,
                             set_property_context->cb_data, &err);
  }

  set_property_context_destroy(set_property_context);

  axis_error_deinit(&err);
}

bool axis_extension_set_property_async(axis_extension_t *self, const char *path,
                                      axis_value_t *value,
                                      axis_extension_set_property_async_cb_t cb,
                                      void *cb_data,
                                      axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is intended to be called in any threads.
  axis_ASSERT(axis_extension_check_integrity(self, false),
             "Invalid use of extension %p.", self);

  axis_extension_set_property_context_t *set_property_context =
      set_property_context_create(path, value, cb, cb_data);

  axis_runloop_post_task_tail(axis_extension_get_attached_runloop(self),
                             axis_extension_set_property_task, self,
                             set_property_context);

  return true;
}

axis_value_t *axis_extension_peek_property(axis_extension_t *extension,
                                         const char *path, axis_error_t *err) {
  axis_ASSERT(extension && axis_extension_check_integrity(extension, true),
             "Invalid argument.");

  if (!path || !strlen(path)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                    "path should not be empty.");
    }
    return NULL;
  }

  axis_extension_thread_t *extension_thread = extension->extension_thread;
  axis_ASSERT(extension_thread, "Invalid argument.");
  axis_ASSERT(axis_extension_thread_check_integrity(extension_thread, true),
             "Invalid use of extension_thread %p.", extension_thread);

  axis_value_t *v = axis_value_peek_from_path(&extension->property, path, err);
  if (!v) {
    return NULL;
  }

  return v;
}

static axis_extension_peek_property_context_t *
axis_extension_peek_property_context_create(
    const char *name, axis_extension_peek_property_async_cb_t cb,
    void *cb_data) {
  axis_extension_peek_property_context_t *context =
      axis_MALLOC(sizeof(axis_extension_peek_property_context_t));
  axis_ASSERT(context, "Failed to allocate memory.");

  axis_string_init_formatted(&context->path, name);
  context->cb = cb;
  context->cb_data = cb_data;
  context->res = NULL;

  return context;
}

static void axis_extension_peek_property_context_destroy(
    axis_extension_peek_property_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->path);

  axis_FREE(self);
}

static void axis_extension_peek_property_task(void *self_, void *arg) {
  axis_extension_t *self = (axis_extension_t *)self_;
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  axis_extension_peek_property_context_t *context =
      (axis_extension_peek_property_context_t *)arg;
  axis_ASSERT(context, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  context->res = axis_extension_peek_property(
      self, axis_string_get_raw_str(&context->path), &err);

  if (context->cb) {
    context->cb(self, context->res, context->cb_data, &err);
  }

  axis_extension_peek_property_context_destroy(context);

  axis_error_deinit(&err);
}

bool axis_extension_peek_property_async(
    axis_extension_t *self, const char *path,
    axis_extension_peek_property_async_cb_t cb, void *cb_data,
    axis_UNUSED axis_error_t *err) {
  axis_ASSERT(
      self && axis_extension_check_integrity(
                  self,
                  // axis_NOLINTNEXTLINE(thread-check)
                  // thread-check: This function maybe called from any thread.
                  false),
      "Invalid argument.");

  axis_extension_peek_property_context_t *context =
      axis_extension_peek_property_context_create(path, cb, cb_data);

  axis_runloop_post_task_tail(self->extension_thread->runloop,
                             axis_extension_peek_property_task, self, context);

  return true;
}

axis_value_t *axis_extension_peek_manifest(axis_extension_t *self,
                                         const char *path, axis_error_t *err) {
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The manifest of an extension is read-only, so it's safe to
  // access it from any threads.
  axis_ASSERT(self && axis_extension_check_integrity(self, false),
             "Invalid argument.");
  axis_ASSERT(path, "Invalid argument.");

  if (!path || !strlen(path)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                    "path should not be empty.");
    }
    return NULL;
  }

  axis_value_t *v = axis_value_peek_from_path(&self->manifest, path, err);
  if (!v) {
    return NULL;
  }

  return v;
}

static axis_extension_peek_manifest_context_t *
axis_extension_peek_manifest_context_create(
    const char *name, axis_extension_peek_manifest_async_cb_t cb,
    void *cb_data) {
  axis_extension_peek_manifest_context_t *context =
      axis_MALLOC(sizeof(axis_extension_peek_manifest_context_t));
  axis_ASSERT(context, "Failed to allocate memory.");

  axis_string_init_formatted(&context->path, name);
  context->cb = cb;
  context->cb_data = cb_data;
  context->res = NULL;

  return context;
}

static void axis_extension_peek_manifest_context_destroy(
    axis_extension_peek_manifest_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->path);

  axis_FREE(self);
}

static void axis_extension_peek_manifest_task(void *self_, void *arg) {
  axis_extension_t *self = (axis_extension_t *)self_;
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Should not happen.");

  axis_extension_peek_manifest_context_t *context =
      (axis_extension_peek_manifest_context_t *)arg;
  axis_ASSERT(context, "Should not happen.");

  axis_error_t err;
  axis_error_init(&err);

  context->res = axis_extension_peek_manifest(
      self, axis_string_get_raw_str(&context->path), &err);

  if (context->cb) {
    context->cb(self, context->res, context->cb_data, &err);
  }

  axis_extension_peek_manifest_context_destroy(context);

  axis_error_deinit(&err);
}

bool axis_extension_peek_manifest_async(
    axis_extension_t *self, const char *path,
    axis_extension_peek_manifest_async_cb_t cb, void *cb_data,
    axis_UNUSED axis_error_t *err) {
  axis_ASSERT(
      self && axis_extension_check_integrity(
                  self,
                  // axis_NOLINTNEXTLINE(thread-check)
                  // thread-check: This function maybe called from any thread.
                  false),
      "Invalid argument.");

  axis_extension_peek_manifest_context_t *context =
      axis_extension_peek_manifest_context_create(path, cb, cb_data);

  axis_runloop_post_task_tail(self->extension_thread->runloop,
                             axis_extension_peek_manifest_task, self, context);

  return true;
}
