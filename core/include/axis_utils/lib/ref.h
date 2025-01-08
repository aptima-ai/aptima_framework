//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/signature.h"

#define axis_REF_SIGNATURE 0x759D8D9D2661E36BU

typedef struct axis_ref_t axis_ref_t;

typedef void (*axis_ref_on_end_of_life_func_t)(axis_ref_t *ref, void *supervisee);

typedef struct axis_ref_t {
  axis_signature_t signature;

  axis_atomic_t ref_cnt;

  // The object which is managed by this 'axis_ref_t'. This field should _not_ be
  // modified after 'axis_ref_t' has been initted, therefore, we don't need to
  // care about its thread safety.
  void *supervisee;

  // This function will be called when the end-of-life of 'supervisee' is
  // reached. This field should _not_ be modified after 'axis_ref_t' has been
  // initted, therefore, we don't need to care about its thread safety.
  axis_ref_on_end_of_life_func_t on_end_of_life;
} axis_ref_t;

/**
 * @param on_end_of_line Required. If the 'axis_ref_t' object in @a supervisee is
 * a pointer, you _must_ call 'axis_ref_destroy()' in @a on_end_of_life. And if
 * the 'axis_ref_t' object is an embedded field in @a supervisee, you _must_ call
 * 'axis_ref_deinit()' in @a on_end_of_life.
 */
axis_UTILS_API axis_ref_t *axis_ref_create(
    void *supervisee, axis_ref_on_end_of_life_func_t on_end_of_life);

/**
 * @brief No matter what the ref_cnt is, force terminate axis_ref_t.
 *
 * @note Use with caution.
 */
axis_UTILS_API void axis_ref_destroy(axis_ref_t *self);

/**
 * @param on_end_of_line Required. If the 'axis_ref_t' object in @a supervisee is
 * a pointer, you _must_ call 'axis_ref_destroy()' in @a on_end_of_life. And if
 * the 'axis_ref_t' object is an embedded field in @a supervisee, you _must_ call
 * 'axis_ref_deinit()' in @a on_end_of_life.
 */
axis_UTILS_API void axis_ref_init(axis_ref_t *self, void *supervisee,
                                axis_ref_on_end_of_life_func_t on_end_of_life);

axis_UTILS_API void axis_ref_deinit(axis_ref_t *self);

axis_UTILS_API bool axis_ref_inc_ref(axis_ref_t *self);

axis_UTILS_API bool axis_ref_dec_ref(axis_ref_t *self);

axis_UTILS_API int64_t axis_ref_get_ref(axis_ref_t *self);
