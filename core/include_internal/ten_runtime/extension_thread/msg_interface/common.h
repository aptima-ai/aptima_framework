//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "ten_utils/lib/smart_ptr.h"

typedef struct ten_extension_thread_t ten_extension_thread_t;

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_handle_msg_async(
    ten_extension_thread_t *self, ten_shared_ptr_t *msg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_dispatch_msg(
    ten_extension_thread_t *self, ten_shared_ptr_t *msg);

TEN_RUNTIME_PRIVATE_API void ten_extension_thread_handle_start_msg_task(
    void *self_, void *arg);
