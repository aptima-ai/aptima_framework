//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_utils/lib/smart_ptr.h"

typedef struct ten_engine_t ten_engine_t;

TEN_RUNTIME_PRIVATE_API void ten_engine_append_to_in_msgs_queue(
    ten_engine_t *self, ten_shared_ptr_t *cmd);

TEN_RUNTIME_PRIVATE_API void ten_engine_handle_in_msgs_async(ten_engine_t *self);

TEN_RUNTIME_PRIVATE_API void ten_engine_handle_msg(ten_engine_t *self,
                                                 ten_shared_ptr_t *msg);

TEN_RUNTIME_PRIVATE_API void ten_engine_dispatch_msg(ten_engine_t *self,
                                                   ten_shared_ptr_t *msg);
