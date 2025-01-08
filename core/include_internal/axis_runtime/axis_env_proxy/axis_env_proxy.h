//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>
#include <stddef.h>

#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/thread.h"

#define axis_PROXY_SIGNATURE 0x31C9BF243A8A65BEU

typedef struct axis_env_t axis_env_t;

typedef struct axis_env_proxy_t {
  axis_signature_t signature;

  axis_mutex_t *lock;

  axis_thread_t *acquired_lock_mode_thread;
  size_t thread_cnt;
  axis_env_t *axis_env;
} axis_env_proxy_t;

typedef struct axis_notify_data_t {
  axis_env_proxy_notify_func_t notify_func;
  void *user_data;
} axis_notify_data_t;

axis_RUNTIME_API bool axis_env_proxy_check_integrity(axis_env_proxy_t *self);

axis_RUNTIME_API size_t axis_env_proxy_get_thread_cnt(axis_env_proxy_t *self,
                                                    axis_error_t *err);

axis_RUNTIME_PRIVATE_API bool axis_env_proxy_acquire(axis_env_proxy_t *self,
                                                   axis_error_t *err);

axis_RUNTIME_API bool axis_env_proxy_notify_async(
    axis_env_proxy_t *self, axis_env_proxy_notify_func_t notify_func,
    void *user_data, axis_error_t *err);
