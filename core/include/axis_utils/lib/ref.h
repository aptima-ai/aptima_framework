//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/lib/atomic.h"
#include "aptima_utils/lib/signature.h"

#define aptima_REF_SIGNATURE 0x759D8D9D2661E36BU

typedef struct aptima_ref_t aptima_ref_t;

typedef void (*aptima_ref_on_end_of_life_func_t)(aptima_ref_t *ref, void *supervisee);

typedef struct aptima_ref_t {
  aptima_signature_t signature;

  aptima_atomic_t ref_cnt;

  // The object which is managed by this 'aptima_ref_t'. This field should _not_ be
  // modified after 'aptima_ref_t' has been initted, therefore, we don't need to
  // care about its thread safety.
  void *supervisee;

  // This function will be called when the end-of-life of 'supervisee' is
  // reached. This field should _not_ be modified after 'aptima_ref_t' has been
  // initted, therefore, we don't need to care about its thread safety.
  aptima_ref_on_end_of_life_func_t on_end_of_life;
} aptima_ref_t;

/**
 * @param on_end_of_line Required. If the 'aptima_ref_t' object in @a supervisee is
 * a pointer, you _must_ call 'aptima_ref_destroy()' in @a on_end_of_life. And if
 * the 'aptima_ref_t' object is an embedded field in @a supervisee, you _must_ call
 * 'aptima_ref_deinit()' in @a on_end_of_life.
 */
aptima_UTILS_API aptima_ref_t *aptima_ref_create(
    void *supervisee, aptima_ref_on_end_of_life_func_t on_end_of_life);

/**
 * @brief No matter what the ref_cnt is, force terminate aptima_ref_t.
 *
 * @note Use with caution.
 */
aptima_UTILS_API void aptima_ref_destroy(aptima_ref_t *self);

/**
 * @param on_end_of_line Required. If the 'aptima_ref_t' object in @a supervisee is
 * a pointer, you _must_ call 'aptima_ref_destroy()' in @a on_end_of_life. And if
 * the 'aptima_ref_t' object is an embedded field in @a supervisee, you _must_ call
 * 'aptima_ref_deinit()' in @a on_end_of_life.
 */
aptima_UTILS_API void aptima_ref_init(aptima_ref_t *self, void *supervisee,
                                aptima_ref_on_end_of_life_func_t on_end_of_life);

aptima_UTILS_API void aptima_ref_deinit(aptima_ref_t *self);

aptima_UTILS_API bool aptima_ref_inc_ref(aptima_ref_t *self);

aptima_UTILS_API bool aptima_ref_dec_ref(aptima_ref_t *self);

aptima_UTILS_API int64_t aptima_ref_get_ref(aptima_ref_t *self);
