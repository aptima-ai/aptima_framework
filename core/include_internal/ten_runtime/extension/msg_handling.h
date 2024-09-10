//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/smart_ptr.h"

typedef struct ten_extension_t ten_extension_t;
typedef struct ten_msg_conversion_operation_t ten_msg_conversion_operation_t;

TEN_RUNTIME_PRIVATE_API void ten_extension_handle_in_msg(ten_extension_t *self,
                                                       ten_shared_ptr_t *msg);
