//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/buf.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "include_internal/axis_utils/lib/buf.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

bool axis_buf_check_integrity(axis_buf_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) != axis_BUF_SIGNATURE) {
    return false;
  }

  return true;
}

static void axis_buf_init_empty(axis_buf_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_signature_set(&self->signature, axis_BUF_SIGNATURE);

  self->data = NULL;
  self->content_size = 0;
  self->size = 0;
  self->owns_memory = true;
  self->is_fixed_size = false;
}

void axis_buf_deinit(axis_buf_t *self) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid argument.");

  if (self->owns_memory && self->data) {
    axis_FREE(self->data);
  }

  axis_signature_set(&self->signature, 0);

  self->data = NULL;
  self->content_size = 0;
  self->size = 0;
  self->owns_memory = true;
  self->is_fixed_size = false;
}

void axis_buf_reset(axis_buf_t *self) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid arguments.");

  if (self->size > 0) {
    if (self->owns_memory) {
      memset(self->data, 0, self->size);
    } else {
      self->data = NULL;
      self->size = 0;
    }
  }

  self->content_size = 0;
  self->is_fixed_size = false;
}

bool axis_buf_init_with_owned_data(axis_buf_t *self, size_t size) {
  axis_ASSERT(self, "Invalid argument.");
  if (!self) {
    return false;
  }

  axis_buf_init_empty(self);

  if (size != 0) {
    self->data = axis_MALLOC(size);
    self->size = size;
  } else {
    self->data = NULL;
    self->size = 0;
  }

  self->owns_memory = true;

  return true;
}

bool axis_buf_init_with_unowned_data(axis_buf_t *self, uint8_t *data,
                                    size_t size) {
  axis_ASSERT(self, "Invalid argument.");
  if (!self) {
    return false;
  }

  axis_buf_init_empty(self);

  self->data = data;
  self->content_size = size;
  self->size = size;

  self->owns_memory = false;

  return true;
}

bool axis_buf_init_with_copying_data(axis_buf_t *self, uint8_t *data,
                                    size_t size) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(data, "Invalid argument.");
  axis_ASSERT(size, "Invalid argument.");
  if (!self) {
    return false;
  }

  axis_buf_init_empty(self);

  self->data = axis_MALLOC(size);
  axis_ASSERT(self->data, "Failed to allocate memory.");

  memcpy(self->data, data, size);
  self->content_size = size;
  self->size = size;

  self->owns_memory = true;

  return true;
}

axis_buf_t *axis_buf_create_with_owned_data(size_t size) {
  axis_buf_t *self = axis_MALLOC(sizeof(axis_buf_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  if (!axis_buf_init_with_owned_data(self, size)) {
    axis_FREE(self);
    return NULL;
  }

  return self;
}

void axis_buf_destroy(axis_buf_t *self) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid argument.");

  axis_buf_deinit(self);
  axis_FREE(self);
}

void axis_buf_set_fixed_size(axis_buf_t *self, bool fixed) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid argument.");
  axis_ASSERT(self->owns_memory,
             "Should not change the size of unowned buffer.");

  self->is_fixed_size = fixed;
}

bool axis_buf_reserve(axis_buf_t *self, size_t len) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid argument.");
  axis_ASSERT(self->owns_memory,
             "Should not change the size of unowned buffer.");

  size_t req_size = self->content_size + len;
  if (req_size >= self->size) {
    if (self->is_fixed_size) {
      axis_LOGE("Attempt to reserve more memory for a fixed-sized axis_buf_t.");
      axis_ASSERT(0, "Should not happen.");
      return false;
    }

    if (self->size == 0) {
      self->size = req_size;
    }

    while (req_size >= self->size) {
      self->size *= 2;
    }

    self->data = axis_REALLOC(self->data, self->size);
    axis_ASSERT(self->data, "Failed to allocate memory.");
  }

  return true;
}

bool axis_buf_push(axis_buf_t *self, const uint8_t *src, size_t size) {
  axis_ASSERT(self && axis_buf_check_integrity(self) && src, "Invalid argument.");
  axis_ASSERT(self->owns_memory,
             "Should not change the size of unowned buffer.");

  if (!self) {
    return false;
  }

  if (!size) {
    return false;
  }

  if (!axis_buf_reserve(self, size)) {
    return false;
  }

  memcpy((char *)(self->data) + self->content_size, (char *)src, size);
  self->content_size += size;

  return true;
}

bool axis_buf_pop(axis_buf_t *self, uint8_t *dest, size_t size) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid argument.");
  axis_ASSERT(self->owns_memory,
             "Should not change the size of unowned buffer.");

  if (!self) {
    return false;
  }

  if (!size) {
    return true;
  }

  if (size > self->content_size) {
    axis_LOGW("Attempt to pop too many bytes from axis_buf_t.");
    return false;
  }

  size_t new_size = self->content_size - size;
  if (dest) {
    memcpy(dest, &self->data[new_size], size);
  }
  self->content_size = new_size;

  // Clear garbage in the discarded storage.
  memset(&self->data[new_size], 0, size);

  return true;
}

bool axis_buf_get_back(axis_buf_t *self, uint8_t *dest, size_t size) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid argument.");
  if (!self) {
    return false;
  }

  if (!size) {
    return true;
  }

  if (!dest) {
    axis_LOGD("Attempt peek into a null pointer");
    return false;
  }

  if (size > self->content_size) {
    axis_LOGW("Attempt to peek too many bytes from axis_buf_t.");
    return false;
  }

  memcpy(dest, &self->data[self->content_size - size], size);

  return true;
}

void axis_buf_take_ownership(axis_buf_t *self) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid arguments.");
  self->owns_memory = true;
}

void axis_buf_release_ownership(axis_buf_t *self) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid arguments.");
  self->owns_memory = false;
}

size_t axis_buf_get_content_size(axis_buf_t *self) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid argument.");
  return self->content_size;
}

size_t axis_buf_get_size(axis_buf_t *self) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid argument.");
  return self->size;
}

uint8_t *axis_buf_get_data(axis_buf_t *self) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid argument.");
  return self->data;
}

void axis_buf_move(axis_buf_t *self, axis_buf_t *other) {
  axis_ASSERT(self && axis_buf_check_integrity(self), "Invalid argument.");
  axis_ASSERT(other && axis_buf_check_integrity(self), "Invalid argument.");

  self->data = other->data;
  self->content_size = other->content_size;
  self->size = other->size;
  self->owns_memory = other->owns_memory;
  self->is_fixed_size = other->is_fixed_size;

  // The buf_t 'other' being moved to 'self' means that the ownership of the
  // memory has been transferred.
  other->owns_memory = false;

  axis_buf_init_with_owned_data(other, 0);
}
