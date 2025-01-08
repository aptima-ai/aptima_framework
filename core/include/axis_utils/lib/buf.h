//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "axis_utils/lib/signature.h"

#define axis_BUF_SIGNATURE 0x909BBC97B47EC291U

#define axis_BUF_STATIC_INIT_OWNED \
  (axis_buf_t) { axis_BUF_SIGNATURE, NULL, 0, 0, true, false }

#define axis_BUF_STATIC_INIT_UNOWNED \
  (axis_buf_t) { axis_BUF_SIGNATURE, NULL, 0, 0, false, false }

#define axis_BUF_STATIC_INIT_WITH_DATA_OWNED(data, size) \
  (axis_buf_t) { axis_BUF_SIGNATURE, data, size, size, true, false }

#define axis_BUF_STATIC_INIT_WITH_DATA_UNOWNED(data, size) \
  (axis_buf_t) { axis_BUF_SIGNATURE, data, size, size, false, false }

// The concept of buf_t is simple: if a memory buffer is passed in during
// construction, then buf_t will not own that memory buffer because buf_t does
// not know how to release it. Using any memory release API in buf_t's
// destructor could potentially mismatch with the API the user originally used
// to create that memory buffer. Therefore, if buf_t's constructor parameters
// are void* and size_t, the behavior is not owning that memory buffer, as the
// destructor would not know how to free/delete/release that buffer. Conversely,
// if buf_t's constructor parameter is just a size, it implies that buf_t will
// internally create the memory buffer, and in this case, buf_t will have
// ownership of that memory buffer because buf_t knows how to release it. On the
// other hand, if the user were to release the buffer themselves, it would
// result in a similar mismatch between creation and release APIs.
typedef struct axis_buf_t {
  axis_signature_t signature;

  uint8_t *data;  //< The pointer points to the beginning of the buffer.
  size_t size;    //< The size of the allocated buffer.

  size_t content_size;  //< The size of the actual data contained in the buffer.
  bool owns_memory;     //< Whether if 'axis_buf_t' owns the containing buffer.
  bool is_fixed_size;   //< Whether if the size of the buffer is fixed.
} axis_buf_t;

axis_UTILS_API bool axis_buf_check_integrity(axis_buf_t *self);

axis_UTILS_API void axis_buf_reset(axis_buf_t *self);

/**
 * @brief Init a buffer.
 * @param size The size of buffer.
 */
axis_UTILS_API bool axis_buf_init_with_owned_data(axis_buf_t *self, size_t size);

/**
 * @brief Initialize a axis_buf_t with @a size bytes of data.
 *
 * @param data The pointer of buffer.
 * @param size The size of buffer.
 */
axis_UTILS_API bool axis_buf_init_with_unowned_data(axis_buf_t *self,
                                                  uint8_t *data, size_t size);

axis_UTILS_API bool axis_buf_init_with_copying_data(axis_buf_t *self,
                                                  uint8_t *data, size_t size);

/**
 * @brief Create a axis_buf_t with @a size bytes.
 */
axis_UTILS_API axis_buf_t *axis_buf_create_with_owned_data(size_t size);

/**
 * @brief De-init a axis_buf_t struct
 *
 * @param self The buf.
 */
axis_UTILS_API void axis_buf_deinit(axis_buf_t *self);

/**
 * @brief Destroy a axis_buf_t struct
 *
 * @param self The buf.
 */
axis_UTILS_API void axis_buf_destroy(axis_buf_t *self);

axis_UTILS_API void axis_buf_move(axis_buf_t *self, axis_buf_t *other);

axis_UTILS_API uint8_t *axis_buf_get_data(axis_buf_t *self);

axis_UTILS_API size_t axis_buf_get_size(axis_buf_t *self);
