//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

typedef struct ten_extension_t ten_extension_t;
typedef struct ten_timer_t ten_timer_t;

TEN_RUNTIME_PRIVATE_API void ten_extension_do_pre_close_action(
    ten_extension_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_timer_closed(ten_timer_t *timer,
                                                         void *on_closed_data);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_path_timer_closed(
    ten_timer_t *timer, void *on_closed_data);
