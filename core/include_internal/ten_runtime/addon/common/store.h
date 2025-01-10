//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/addon/addon.h"
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

TEN_RUNTIME_PRIVATE_API ten_addon_t *ten_addon_store_del(
    ten_addon_store_t *store, const char *name);

TEN_RUNTIME_PRIVATE_API void ten_addon_store_del_all(ten_addon_store_t *store);

TEN_RUNTIME_PRIVATE_API ten_addon_host_t *ten_addon_store_find(
    ten_addon_store_t *store, const char *name);

TEN_RUNTIME_PRIVATE_API ten_addon_host_t *
ten_addon_store_find_or_create_one_if_not_found(ten_addon_store_t *store,
                                                TEN_ADDON_TYPE addon_type,
                                                const char *addon_name,
                                                bool *newly_created);
