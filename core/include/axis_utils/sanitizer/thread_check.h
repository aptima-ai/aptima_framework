//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/lib/signature.h"
#include "aptima_utils/lib/thread.h"

#define aptima_SANITIZER_THREAD_CHECK_SIGNATURE 0x6204388773560E59U

typedef struct aptima_sanitizer_thread_check_t {
  aptima_signature_t signature;

  aptima_thread_t *belonging_thread;
  bool is_fake;
} aptima_sanitizer_thread_check_t;

aptima_UTILS_PRIVATE_API bool aptima_sanitizer_thread_check_check_integrity(
    aptima_sanitizer_thread_check_t *self);

aptima_UTILS_API void aptima_sanitizer_thread_check_init_with_current_thread(
    aptima_sanitizer_thread_check_t *self);

aptima_UTILS_API void aptima_sanitizer_thread_check_init_from(
    aptima_sanitizer_thread_check_t *self, aptima_sanitizer_thread_check_t *other);

aptima_UTILS_API aptima_thread_t *aptima_sanitizer_thread_check_get_belonging_thread(
    aptima_sanitizer_thread_check_t *self);

aptima_UTILS_API void aptima_sanitizer_thread_check_set_belonging_thread(
    aptima_sanitizer_thread_check_t *self, aptima_thread_t *thread);

aptima_UTILS_API void
aptima_sanitizer_thread_check_set_belonging_thread_to_current_thread(
    aptima_sanitizer_thread_check_t *self);

aptima_UTILS_API void aptima_sanitizer_thread_check_inherit_from(
    aptima_sanitizer_thread_check_t *self, aptima_sanitizer_thread_check_t *from);

aptima_UTILS_API bool aptima_sanitizer_thread_check_do_check(
    aptima_sanitizer_thread_check_t *self);

aptima_UTILS_API void aptima_sanitizer_thread_check_deinit(
    aptima_sanitizer_thread_check_t *self);
