//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from https://github.com/gpakosz/uuid4/
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "axis_utils/lib/string.h"

typedef uint64_t axis_uuid4_state_t;

typedef union axis_uuid4_t {
  uint8_t bytes[16];
  uint32_t dwords[4];
  uint64_t qwords[2];
} axis_uuid4_t;

axis_UTILS_PRIVATE_API uint32_t axis_uuid4_mix(uint32_t a, uint32_t b);

axis_UTILS_PRIVATE_API uint32_t axis_uuid4_hash(uint32_t value);

/**
 * Seeds the state of the PRNG used to generate version 4 UUIDs.
 *
 * @param a pointer to a variable holding the state.
 *
 * @return `true` on success, otherwise `false`.
 */
axis_UTILS_API void axis_uuid4_seed(axis_uuid4_state_t *seed);

axis_UTILS_API void axis_uuid4_init_to_zeros(axis_uuid4_t *self);

/**
 * Generates a version 4 UUID, see https://tools.ietf.org/html/rfc4122.
 *
 * @param state the state of the PRNG used to generate version 4 UUIDs.
 * @param out the recipient for the UUID.
 */
axis_UTILS_PRIVATE_API void axis_uuid4_gen_from_seed(axis_uuid4_t *self,
                                                   axis_uuid4_state_t *seed);

axis_UTILS_API void axis_uuid4_gen(axis_uuid4_t *out);

axis_UTILS_API void axis_uuid4_gen_string(axis_string_t *out);

axis_UTILS_API bool axis_uuid4_is_equal(const axis_uuid4_t *a,
                                      const axis_uuid4_t *b);

axis_UTILS_API void axis_uuid4_copy(axis_uuid4_t *self, axis_uuid4_t *src);

axis_UTILS_API bool axis_uuid4_is_empty(axis_uuid4_t *self);

axis_UTILS_API bool axis_uuid4_from_string(axis_uuid4_t *self, axis_string_t *in);

/**
 * Converts a UUID to a `NUL` terminated string.
 * The string format is like 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx', y is either
 * 8, 9, a or b
 *
 * @param out destination ten string
 *
 * @return `true` on success, otherwise `false`.
 */
axis_UTILS_API bool axis_uuid4_to_string(const axis_uuid4_t *self,
                                       axis_string_t *out);
