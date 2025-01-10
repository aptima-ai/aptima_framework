//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

/**
 * @brief A view of the contents of a file. This supports mmap when available.
 * A view will remain in memory even after aptima_file_close is called on the
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
typedef struct aptima_mmap_t {
  const void *data;  // The data that the caller requested.
  void *base;        // The base of the view.
  size_t len;        // The total length of the view.
} aptima_mmap_t;

/**
 * @brief Create a view of @a size bytes from @a descriptor at @a offset. Store
 * the result in @a *view.
 *
 * @return 1 on success, 0 on error.
 */
aptima_UTILS_PRIVATE_API bool aptima_mmap_init(aptima_mmap_t *self, int descriptor,
                                         off_t offset, uint64_t size);

/**
 * @brief Release a view created by aptima_mmap_init.
 */
aptima_UTILS_PRIVATE_API void aptima_mmap_deinit(aptima_mmap_t *self);
