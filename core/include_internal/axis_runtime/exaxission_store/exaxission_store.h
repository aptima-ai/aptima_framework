//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_utils/container/hash_handle.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_EXTENSION_STORE_SIGNATURE 0x73826F288A62F9D2U

typedef struct axis_extension_t axis_extension_t;

typedef struct axis_extension_store_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_hashtable_t hash_table;
} axis_extension_store_t;

axis_RUNTIME_PRIVATE_API axis_extension_store_t *axis_extension_store_create(
    ptrdiff_t offset);

axis_RUNTIME_PRIVATE_API void axis_extension_store_destroy(
    axis_extension_store_t *self);

axis_RUNTIME_PRIVATE_API bool axis_extension_store_add_extension(
    axis_extension_store_t *self, axis_extension_t *extension);

axis_RUNTIME_PRIVATE_API void axis_extension_store_del_extension(
    axis_extension_store_t *self, axis_extension_t *extension);

axis_RUNTIME_PRIVATE_API axis_extension_t *axis_extension_store_find_extension(
    axis_extension_store_t *self, const char *extension_name, bool check_thread);
