//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef struct ten_engine_t ten_engine_t;
typedef struct ten_app_t ten_app_t;

TEN_RUNTIME_PRIVATE_API void ten_engine_create_its_own_thread(ten_engine_t *self);

TEN_RUNTIME_PRIVATE_API void ten_engine_init_individual_eventloop_relevant_vars(
    ten_engine_t *self, ten_app_t *app);
