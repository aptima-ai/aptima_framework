//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_config.h"

#include "axis_utils/container/list.h"
#include "axis_utils/lib/mutex.h"

typedef struct axis_app_t axis_app_t;

axis_RUNTIME_PRIVATE_API axis_list_t g_apps;
axis_RUNTIME_PRIVATE_API axis_mutex_t *g_apps_mutex;

axis_RUNTIME_API void axis_global_init(void);

axis_RUNTIME_API void axis_global_deinit(void);

axis_RUNTIME_PRIVATE_API void axis_global_add_app(axis_app_t *self);

axis_RUNTIME_PRIVATE_API void axis_global_del_app(axis_app_t *self);
