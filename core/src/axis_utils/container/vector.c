//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/container/vector.h"

#include <assert.h>
#include <stdlib.h>

#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"

void axis_vector_init(axis_vector_t *self, size_t capacity) {
  axis_ASSERT(self, "Invalid argument.");

  self->size = 0;
  self->capacity = capacity;
  self->data = axis_MALLOC(self->capacity);
  axis_ASSERT(self->data, "Failed to allocate memory.");
}

axis_vector_t *axis_vector_create(size_t capacity) {
  axis_vector_t *self = axis_MALLOC(sizeof(axis_vector_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_vector_init(self, capacity);

  return self;
}

/**
 * @brief Free the space managed by @a vec. This will reset @a vec.
 */
void axis_vector_deinit(axis_vector_t *self) {
  axis_ASSERT(self && self->data, "Invalid argument.");

  self->size = 0;
  axis_vector_release_remaining_space(self);
}

void axis_vector_destroy(axis_vector_t *self) {
  axis_ASSERT(self && self->data, "Invalid argument.");

  axis_FREE(self->data);
  axis_FREE(self);
}

/**
 * @brief Grow @a self by @a size bytes.
 *
 *                        grow space
 *                      v------v
 * -----------------------------
 * |                    |      |
 * -----------------------------
 *                      ^
 *                      returned address
 */
void *axis_vector_grow(axis_vector_t *self, size_t size) {
  axis_ASSERT(self, "Invalid argument.");

  size_t remaining = self->capacity - self->size;

  if (size > remaining) {
    size_t alc = 0;

    if (self->size == 0) {
      // Initially allocate 32 spaces with 'size' size.
      alc = 32 * size;
    } else if (self->size >= 4096) {
      // If the vector contains at least 4096 bytes, increase 4096 bytes each
      // time.
      alc = self->size + 4096;
    } else {
      // If the size of the vector is between 0 and 4096, it will be doubled
      // each time.
      alc = 2 * self->size;
    }

    if (alc < self->size + size) {
      alc = self->size + size;
    }

    void *new_base = axis_REALLOC(self->data, alc);
    axis_ASSERT(new_base, "Failed to realloc memory.");
    if (new_base == NULL) {
      return NULL;
    }

    self->data = new_base;
    self->capacity = alc;
  }

  void *ret = (char *)self->data + self->size;
  self->size += size;

  return ret;
}

/**
 * @brief Release any remaining space allocated for @a self.
 */
bool axis_vector_release_remaining_space(axis_vector_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (self->size == 0) {
    // realloc with size 0 is marked as an obsolescent feature, use free()
    // instead.
    axis_FREE(self->data);
    self->data = NULL;

    goto done;
  }

  self->data = axis_REALLOC(self->data, self->size);
  if (self->data == NULL) {
    axis_ASSERT(self->data, "Failed to realloc memory.");
    return false;
  }

done:
  self->capacity = self->size;
  return true;
}

/**
 * @brief Shrink the data in @a self, and take it out of @a self.
 */
void *axis_vector_take_out(axis_vector_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (!axis_vector_release_remaining_space(self)) {
    return NULL;
  }

  void *ret = self->data;

  // Emulate the operation of taking the data out of the vector.
  self->data = NULL;
  self->size = 0;
  self->capacity = 0;

  return ret;
}
