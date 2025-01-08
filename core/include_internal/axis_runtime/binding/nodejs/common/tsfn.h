//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <node_api.h>

#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/ref.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_NODEJS_THREADSAFE_FUNCTION_SIGNATURE 0x1D11D6EF2722D8FBU

#define CREATE_JS_CB_TSFN(axis_tsfn, env, log_name, js_cb, tsfn_proxy_func)   \
  do {                                                                       \
    axis_tsfn =                                                               \
        axis_nodejs_tsfn_create((env), log_name, (js_cb), (tsfn_proxy_func)); \
    axis_ASSERT((axis_tsfn), "Should not happen.");                            \
    axis_nodejs_tsfn_inc_rc((axis_tsfn));                                      \
  } while (0)

typedef struct axis_nodejs_tsfn_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_ref_t ref;  // Used to determine the timing of destroying this TSFN.

  axis_mutex_t *lock;
  axis_string_t name;

  napi_threadsafe_function tsfn;  // The TSFN itself.

  // The JS function which this tsfn calling.
  //
  // Because the JS functions pointed to by thread-safe functions may not
  // necessarily be normal functions existing in the JS world, they could be
  // dynamically created JS functions. The life cycle of dynamically generated
  // JS functions is bound to the thread-safe functions. Therefore, for unified
  // handling, TEN first acquires a reference to the JS function to prevent it
  // from being garbage collected. Then, when the thread-safe function is
  // finalized, TEN releases that reference, allowing the JS function to be
  // garbage collected.
  napi_ref js_func_ref;
} axis_nodejs_tsfn_t;

axis_RUNTIME_API bool axis_nodejs_tsfn_check_integrity(axis_nodejs_tsfn_t *self,
                                                     bool check_thread);

axis_RUNTIME_API axis_nodejs_tsfn_t *axis_nodejs_tsfn_create(
    napi_env env, const char *name, napi_value js_func,
    napi_threadsafe_function_call_js tsfn_proxy_func);

axis_RUNTIME_API void axis_nodejs_tsfn_inc_rc(axis_nodejs_tsfn_t *self);

axis_RUNTIME_API void axis_nodejs_tsfn_dec_rc(axis_nodejs_tsfn_t *self);

axis_RUNTIME_API bool axis_nodejs_tsfn_invoke(axis_nodejs_tsfn_t *rte_tsfn,
                                            void *data);

axis_RUNTIME_API void axis_nodejs_tsfn_release(axis_nodejs_tsfn_t *self);
