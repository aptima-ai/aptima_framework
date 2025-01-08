//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

/**
 * @brief A view of the contents of a file. This supports mmap when available.
 * A view will remain in memory even after axis_file_close is called on the
 * file descriptor from which the view was obtained.
 *
 *        data
 *        v
 * -------------------------------
 * |      |                      |
 * -------------------------------
 * ^
 * base
 *
 * |<-----------len------------->|
 *
 */
typedef struct axis_mmap_t {
  const void *data;  // The data that the caller requested.
  void *base;        // The base of the view.
  size_t len;        // The total length of the view.
} axis_mmap_t;

/**
 * @brief Create a view of @a size bytes from @a descriptor at @a offset. Store
 * the result in @a *view.
 *
 * @return 1 on success, 0 on error.
 */
axis_UTILS_PRIVATE_API bool axis_mmap_init(axis_mmap_t *self, int descriptor,
                                         off_t offset, uint64_t size);

/**
 * @brief Release a view created by axis_mmap_init.
 */
axis_UTILS_PRIVATE_API void axis_mmap_deinit(axis_mmap_t *self);
