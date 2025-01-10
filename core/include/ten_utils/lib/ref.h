//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stdbool.h>

#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/signature.h"

#define TEN_REF_SIGNATURE 0x759D8D9D2661E36BU

typedef struct ten_ref_t ten_ref_t;

typedef void (*ten_ref_on_end_of_life_func_t)(ten_ref_t *ref, void *supervisee);

typedef struct ten_ref_t {
  ten_signature_t signature;

  ten_atomic_t ref_cnt;

  // The object which is managed by this 'ten_ref_t'. This field should _not_ be
  // modified after 'ten_ref_t' has been initted, therefore, we don't need to
  // care about its thread safety.
  void *supervisee;

  // This function will be called when the end-of-life of 'supervisee' is
  // reached. This field should _not_ be modified after 'ten_ref_t' has been
  // initted, therefore, we don't need to care about its thread safety.
  ten_ref_on_end_of_life_func_t on_end_of_life;
} ten_ref_t;

/**
 * @param on_end_of_line Required. If the 'ten_ref_t' object in @a supervisee is
 * a pointer, you _must_ call 'ten_ref_destroy()' in @a on_end_of_life. And if
 * the 'ten_ref_t' object is an embedded field in @a supervisee, you _must_ call
 * 'ten_ref_deinit()' in @a on_end_of_life.
 */
TEN_UTILS_API ten_ref_t *ten_ref_create(
    void *supervisee, ten_ref_on_end_of_life_func_t on_end_of_life);

/**
 * @brief No matter what the ref_cnt is, force terminate ten_ref_t.
 *
 * @note Use with caution.
 */
TEN_UTILS_API void ten_ref_destroy(ten_ref_t *self);

/**
 * @param on_end_of_line Required. If the 'ten_ref_t' object in @a supervisee is
 * a pointer, you _must_ call 'ten_ref_destroy()' in @a on_end_of_life. And if
 * the 'ten_ref_t' object is an embedded field in @a supervisee, you _must_ call
 * 'ten_ref_deinit()' in @a on_end_of_life.
 */
TEN_UTILS_API void ten_ref_init(ten_ref_t *self, void *supervisee,
                                ten_ref_on_end_of_life_func_t on_end_of_life);

TEN_UTILS_API void ten_ref_deinit(ten_ref_t *self);

TEN_UTILS_API bool ten_ref_inc_ref(ten_ref_t *self);

TEN_UTILS_API bool ten_ref_dec_ref(ten_ref_t *self);

TEN_UTILS_API int64_t ten_ref_get_ref(ten_ref_t *self);
