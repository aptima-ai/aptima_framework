//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/buf.h"

axis_UTILS_PRIVATE_API bool axis_buf_get_back(axis_buf_t *self, uint8_t *dest,
                                            size_t size);

/**
 * @brief Append @a data with @a size bytes to the end of @a self. The source
 * buffer will be _copied_ into the buf.
 *
 * @param self The buf.
 */
axis_UTILS_API bool axis_buf_push(axis_buf_t *self, const uint8_t *src,
                                size_t size);

axis_UTILS_PRIVATE_API bool axis_buf_pop(axis_buf_t *self, uint8_t *dest,
                                       size_t size);

/**
 * @brief Ensure buffer has room for size @a len, grows buffer size if needed.
 *
 * @param self The buf.
 */
axis_UTILS_API bool axis_buf_reserve(axis_buf_t *self, size_t len);

axis_UTILS_API void axis_buf_set_fixed_size(axis_buf_t *self, bool fixed);

axis_UTILS_API size_t axis_buf_get_content_size(axis_buf_t *self);

axis_UTILS_API void axis_buf_take_ownership(axis_buf_t *self);

axis_UTILS_API void axis_buf_release_ownership(axis_buf_t *self);
