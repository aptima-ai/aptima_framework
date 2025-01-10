//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_runtime/addon/addon.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/mutex.h"

typedef struct axis_addon_host_t axis_addon_host_t;
typedef struct axis_addon_t axis_addon_t;

typedef struct axis_addon_store_t {
  axis_atomic_t valid;
  axis_mutex_t *lock;
  axis_list_t store;  // axis_addon_host_t
} axis_addon_store_t;

axis_RUNTIME_PRIVATE_API void axis_addon_store_init(axis_addon_store_t *store);

axis_RUNTIME_PRIVATE_API axis_addon_t *axis_addon_store_del(
    axis_addon_store_t *store, const char *name);

axis_RUNTIME_PRIVATE_API void axis_addon_store_del_all(axis_addon_store_t *store);

axis_RUNTIME_PRIVATE_API axis_addon_host_t *axis_addon_store_find(
    axis_addon_store_t *store, const char *name);

axis_RUNTIME_PRIVATE_API axis_addon_host_t *
axis_addon_store_find_or_create_one_if_not_found(axis_addon_store_t *store,
                                                axis_ADDON_TYPE addon_type,
                                                const char *addon_name,
                                                bool *newly_created);
