//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from https://github.com/gpakosz/uuid4/
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "aptima_utils/lib/string.h"

typedef uint64_t aptima_uuid4_state_t;

typedef union aptima_uuid4_t {
  uint8_t bytes[16];
  uint32_t dwords[4];
  uint64_t qwords[2];
} aptima_uuid4_t;

aptima_UTILS_PRIVATE_API uint32_t aptima_uuid4_mix(uint32_t a, uint32_t b);

aptima_UTILS_PRIVATE_API uint32_t aptima_uuid4_hash(uint32_t value);

/**
 * Seeds the state of the PRNG used to generate version 4 UUIDs.
 *
 * @param a pointer to a variable holding the state.
 *
 * @return `true` on success, otherwise `false`.
 */
aptima_UTILS_API void aptima_uuid4_seed(aptima_uuid4_state_t *seed);

aptima_UTILS_API void aptima_uuid4_init_to_zeros(aptima_uuid4_t *self);

/**
 * Generates a version 4 UUID, see https://tools.ietf.org/html/rfc4122.
 *
 * @param state the state of the PRNG used to generate version 4 UUIDs.
 * @param out the recipient for the UUID.
 */
aptima_UTILS_PRIVATE_API void aptima_uuid4_gen_from_seed(aptima_uuid4_t *self,
                                                   aptima_uuid4_state_t *seed);

aptima_UTILS_API void aptima_uuid4_gen(aptima_uuid4_t *out);

aptima_UTILS_API void aptima_uuid4_gen_string(aptima_string_t *out);

aptima_UTILS_API bool aptima_uuid4_is_equal(const aptima_uuid4_t *a,
                                      const aptima_uuid4_t *b);

aptima_UTILS_API void aptima_uuid4_copy(aptima_uuid4_t *self, aptima_uuid4_t *src);

aptima_UTILS_API bool aptima_uuid4_is_empty(aptima_uuid4_t *self);

aptima_UTILS_API bool aptima_uuid4_from_string(aptima_uuid4_t *self, aptima_string_t *in);

/**
 * Converts a UUID to a `NUL` terminated string.
 * The string format is like 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx', y is either
 * 8, 9, a or b
 *
 * @param out destination ten string
 *
 * @return `true` on success, otherwise `false`.
 */
aptima_UTILS_API bool aptima_uuid4_to_string(const aptima_uuid4_t *self,
                                       aptima_string_t *out);
