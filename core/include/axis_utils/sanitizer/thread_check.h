//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/thread.h"

#define axis_SANITIZER_THREAD_CHECK_SIGNATURE 0x6204388773560E59U

typedef struct axis_sanitizer_thread_check_t {
  axis_signature_t signature;

  axis_thread_t *belonging_thread;
  bool is_fake;
} axis_sanitizer_thread_check_t;

axis_UTILS_PRIVATE_API bool axis_sanitizer_thread_check_check_integrity(
    axis_sanitizer_thread_check_t *self);

axis_UTILS_API void axis_sanitizer_thread_check_init_with_current_thread(
    axis_sanitizer_thread_check_t *self);

axis_UTILS_API void axis_sanitizer_thread_check_init_from(
    axis_sanitizer_thread_check_t *self, axis_sanitizer_thread_check_t *other);

axis_UTILS_API axis_thread_t *axis_sanitizer_thread_check_get_belonging_thread(
    axis_sanitizer_thread_check_t *self);

axis_UTILS_API void axis_sanitizer_thread_check_set_belonging_thread(
    axis_sanitizer_thread_check_t *self, axis_thread_t *thread);

axis_UTILS_API void
axis_sanitizer_thread_check_set_belonging_thread_to_current_thread(
    axis_sanitizer_thread_check_t *self);

axis_UTILS_API void axis_sanitizer_thread_check_inherit_from(
    axis_sanitizer_thread_check_t *self, axis_sanitizer_thread_check_t *from);

axis_UTILS_API bool axis_sanitizer_thread_check_do_check(
    axis_sanitizer_thread_check_t *self);

axis_UTILS_API void axis_sanitizer_thread_check_deinit(
    axis_sanitizer_thread_check_t *self);
