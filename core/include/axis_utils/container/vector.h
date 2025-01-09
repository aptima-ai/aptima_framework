//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct aptima_vector_t {
  void *data;
  size_t size;
  size_t capacity;
} aptima_vector_t;

aptima_UTILS_API void aptima_vector_init(aptima_vector_t *self, size_t capacity);

/**
 * Create a new vector of the given capacity.
 */
aptima_UTILS_API aptima_vector_t *aptima_vector_create(size_t capacity);

aptima_UTILS_API void aptima_vector_deinit(aptima_vector_t *self);

/**
 * Free vector related memory.
 */
aptima_UTILS_API void aptima_vector_destroy(aptima_vector_t *self);

aptima_UTILS_API void *aptima_vector_grow(aptima_vector_t *self, size_t size);

aptima_UTILS_API bool aptima_vector_release_remaining_space(aptima_vector_t *self);

aptima_UTILS_API void *aptima_vector_take_out(aptima_vector_t *self);
