//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct axis_vector_t {
  void *data;
  size_t size;
  size_t capacity;
} axis_vector_t;

axis_UTILS_API void axis_vector_init(axis_vector_t *self, size_t capacity);

/**
 * Create a new vector of the given capacity.
 */
axis_UTILS_API axis_vector_t *axis_vector_create(size_t capacity);

axis_UTILS_API void axis_vector_deinit(axis_vector_t *self);

/**
 * Free vector related memory.
 */
axis_UTILS_API void axis_vector_destroy(axis_vector_t *self);

axis_UTILS_API void *axis_vector_grow(axis_vector_t *self, size_t size);

axis_UTILS_API bool axis_vector_release_remaining_space(axis_vector_t *self);

axis_UTILS_API void *axis_vector_take_out(axis_vector_t *self);
