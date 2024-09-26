//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/container/list.h"
#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/mutex.h"

typedef struct ten_addon_host_t ten_addon_host_t;
typedef struct ten_addon_t ten_addon_t;

typedef struct ten_addon_store_t {
  ten_atomic_t valid;
  ten_mutex_t *lock;
  ten_list_t store;  // ten_addon_host_t
} ten_addon_store_t;

TEN_RUNTIME_PRIVATE_API void ten_addon_store_init(ten_addon_store_t *store);

TEN_RUNTIME_PRIVATE_API bool ten_addon_store_add(ten_addon_store_t *store,
                                                 ten_addon_host_t *addon_host);

TEN_RUNTIME_PRIVATE_API ten_addon_t *ten_addon_store_del(
    ten_addon_store_t *store, const char *name);

TEN_RUNTIME_PRIVATE_API ten_addon_host_t *ten_addon_store_find(
    ten_addon_store_t *store, const char *name);
