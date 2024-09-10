//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stddef.h>

#include "ten_utils/lib/buf.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"

typedef struct ten_data_t ten_data_t;

TEN_RUNTIME_API ten_shared_ptr_t *ten_data_create(void);

TEN_RUNTIME_API ten_buf_t *ten_data_peek_buf(ten_shared_ptr_t *self);

TEN_RUNTIME_API void ten_data_set_buf_with_move(ten_shared_ptr_t *self,
                                              ten_buf_t *buf);

TEN_RUNTIME_API uint8_t *ten_data_alloc_buf(ten_shared_ptr_t *self, size_t size);

TEN_RUNTIME_API ten_shared_ptr_t *ten_data_create_from_json_string(
    const char *json_str, ten_error_t *err);
