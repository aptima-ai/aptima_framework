//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/nodejs/common/tsfn.h"

#include "include_internal/axis_runtime/binding/nodejs/common/common.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/macro/memory.h"

static axis_nodejs_tsfn_t *axis_nodejs_tsfn_create_empty(void) {
  axis_nodejs_tsfn_t *self = axis_MALLOC(sizeof(axis_nodejs_tsfn_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, axis_NODEJS_THREADSAFE_FUNCTION_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  axis_string_init(&self->name);
  self->js_func_ref = NULL;
  self->tsfn = NULL;
  self->lock = axis_mutex_create();

  return self;
}

/**
 * @brief This function will be called in the JS main thread.
 */
static void axis_nodejs_tsfn_finalize(napi_env env, void *finalize_data,
                                     axis_UNUSED void *finalize_hint) {
  axis_ASSERT(env, "Should not happen.");

  axis_nodejs_tsfn_t *tsfn_bridge = finalize_data;
  axis_ASSERT(tsfn_bridge && axis_nodejs_tsfn_check_integrity(tsfn_bridge, true),
             "Should not happen.");

  axis_LOGD("TSFN %s is finalized.", axis_string_get_raw_str(&tsfn_bridge->name));

  // The tsfn would be accessed from the native part, so we need to protect the
  // operations of it.
  axis_mutex_lock(tsfn_bridge->lock);
  // Indicate that the tsfn is disappear.
  tsfn_bridge->tsfn = NULL;
  axis_mutex_unlock(tsfn_bridge->lock);

  // Release the reference to the JS function which this tsfn points to.
  axis_ASSERT(tsfn_bridge->js_func_ref, "Should not happen.");
  uint32_t js_func_ref_cnt = 0;
  napi_status status =
      napi_reference_unref(env, tsfn_bridge->js_func_ref, &js_func_ref_cnt);
  ASSERT_IF_NAPI_FAIL(
      status == napi_ok,
      "Failed to release JS func ref pointed by TSFN \"%s\" (%d)",
      axis_string_get_raw_str(&tsfn_bridge->name), js_func_ref_cnt);
  axis_ASSERT(js_func_ref_cnt == 0,
             "The reference count, to the JS func, hold by tsfn should be 1.");

  axis_LOGD(
      "Release JS func ref pointed by TSFN \"%s\", its new ref count is %d",
      axis_string_get_raw_str(&tsfn_bridge->name), js_func_ref_cnt);

  status = napi_delete_reference(env, tsfn_bridge->js_func_ref);
  ASSERT_IF_NAPI_FAIL(status == napi_ok,
                      "Failed to delete JS func ref pointed by TSFN \"%s\"",
                      axis_string_get_raw_str(&tsfn_bridge->name));

  // Indicate that the JS tsfn has destroyed.
  axis_nodejs_tsfn_dec_rc(tsfn_bridge);
}

static void axis_nodejs_tsfn_destroy(axis_nodejs_tsfn_t *self) {
  axis_ASSERT(
      self &&
          // axis_NOLINTNEXTLINE(thread-check)
          // thread-check: if reaching here, it means all the user of the tsfn
          // has ended, so it's safe to call this function in any threads.
          axis_nodejs_tsfn_check_integrity(self, false) &&
          // Before being destroyed, the TSFN should have already been
          // finalized.
          !self->tsfn,
      "Should not happen.");

  axis_string_deinit(&self->name);
  if (self->lock) {
    axis_mutex_destroy(self->lock);
    self->lock = NULL;
  }

  axis_sanitizer_thread_check_deinit(&self->thread_check);

  axis_FREE(self);
}

static void axis_nodejs_tsfn_on_end_of_life(axis_UNUSED axis_ref_t *ref,
                                           void *self_) {
  axis_nodejs_tsfn_t *self = (axis_nodejs_tsfn_t *)self_;

  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The belonging thread of the 'client' is ended when this
  // function is called, so we can not check thread integrity here.
  axis_ASSERT(self && axis_nodejs_tsfn_check_integrity(self, false),
             "Invalid argument.");

  axis_ref_deinit(&self->ref);
  axis_nodejs_tsfn_destroy(self);
}

bool axis_nodejs_tsfn_check_integrity(axis_nodejs_tsfn_t *self,
                                     bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      axis_NODEJS_THREADSAFE_FUNCTION_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

axis_nodejs_tsfn_t *axis_nodejs_tsfn_create(
    napi_env env, const char *name, napi_value js_func,
    napi_threadsafe_function_call_js tsfn_proxy_func) {
  axis_ASSERT(env && name && js_func && tsfn_proxy_func, "Should not happen.");

  axis_nodejs_tsfn_t *self = axis_nodejs_tsfn_create_empty();

  napi_status status = napi_ok;

  // Create a JS reference to keep the JS function alive.
  status = napi_create_reference(env, js_func, 1 /* Initial reference count */,
                                 &self->js_func_ref);
  ASSERT_IF_NAPI_FAIL(status == napi_ok && self->js_func_ref != NULL,
                      "Failed to create reference to JS function %p: %d",
                      js_func, status);

  // Create a name to represent this work. This is must, otherwise,
  // 'napi_create_threadsafe_function' will fail because of this.
  napi_value work_name = NULL;
  status = napi_create_string_utf8(env, name, strlen(name), &work_name);
  ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create JS string: %d",
                      status);

  axis_string_init_formatted(&self->name, "%s", name);

  // Create a TSFN for the corresponding javascript function 'js_func'.
  status = napi_create_threadsafe_function(
      env, js_func, NULL, work_name, 0, 1 /* Initial thread count */, self,
      axis_nodejs_tsfn_finalize, NULL, tsfn_proxy_func, &self->tsfn);
  ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to create TSFN: %d", status);

  // Indicate that the JS part takes one ownership of this tsfn bridge.
  axis_ref_init(&self->ref, self, axis_nodejs_tsfn_on_end_of_life);

  return self;
}

void axis_nodejs_tsfn_inc_rc(axis_nodejs_tsfn_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: this function is meant to be called in any
                 // threads, and all operations in this function is thread-safe.
                 axis_nodejs_tsfn_check_integrity(self, false),
             "Should not happen.");

  axis_ref_inc_ref(&self->ref);
}

void axis_nodejs_tsfn_dec_rc(axis_nodejs_tsfn_t *self) {
  axis_ASSERT(self &&
                 // axis_NOLINTNEXTLINE(thread-check)
                 // thread-check: this function is meant to be called in any
                 // threads, and all operations in this function is thread-safe.
                 axis_nodejs_tsfn_check_integrity(self, false),
             "Should not happen.");

  axis_ref_dec_ref(&self->ref);
}

bool axis_nodejs_tsfn_invoke(axis_nodejs_tsfn_t *self, void *data) {
  axis_ASSERT(self, "Should not happen.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: this function is meant to be called in all threads.
  axis_ASSERT(axis_nodejs_tsfn_check_integrity(self, false),
             "Should not happen.");

  bool result = true;

  // Because rte_nodejs_tsfn_t would be accessed in the JS main
  // thread at any time, so we need a locking here.
  axis_mutex_lock(self->lock);

  if (self->tsfn == NULL) {
    axis_LOGW("Failed to call tsfn %s, because it has been finalized.",
             axis_string_get_raw_str(&self->name));

    result = false;
    goto done;
  }

  napi_status status =
      napi_call_threadsafe_function(self->tsfn, data, napi_tsfn_blocking);
  if (status != napi_ok) {
    if (status == napi_closing) {
      axis_LOGE("Failed to call a destroyed JS thread-safe function %s.",
               axis_string_get_raw_str(&self->name));
    } else {
      axis_LOGE("Failed to call a JS thread-safe function %s: status: %d",
               axis_string_get_raw_str(&self->name), status);
    }

    result = false;
    goto done;
  }

done:
  axis_mutex_unlock(self->lock);
  return result;
}

void axis_nodejs_tsfn_release(axis_nodejs_tsfn_t *self) {
  axis_ASSERT(self && axis_nodejs_tsfn_check_integrity(self, true),
             "Should not happen.");

  axis_LOGD("Release TSFN \"%s\" (%p)", axis_string_get_raw_str(&self->name),
           self->tsfn);

  // 'releasing' the TSFN, so that it can be garbage collected.
  napi_status status =
      napi_release_threadsafe_function(self->tsfn, napi_tsfn_abort);
  ASSERT_IF_NAPI_FAIL(status == napi_ok, "Failed to release TSFN: %d", status);
}
