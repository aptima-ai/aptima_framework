//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_runtime/ten_config.h"

#include "ten_utils/container/list.h"
#include "ten_utils/lib/mutex.h"

typedef struct ten_app_t ten_app_t;

TEN_RUNTIME_PRIVATE_API ten_list_t g_apps;
TEN_RUNTIME_PRIVATE_API ten_mutex_t *g_apps_mutex;

TEN_RUNTIME_API void ten_global_init(void);

TEN_RUNTIME_API void ten_global_deinit(void);

TEN_RUNTIME_PRIVATE_API void ten_global_add_app(ten_app_t *self);

TEN_RUNTIME_PRIVATE_API void ten_global_del_app(ten_app_t *self);
