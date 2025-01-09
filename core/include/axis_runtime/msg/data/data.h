//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <stddef.h>

#include "aptima_utils/lib/buf.h"
#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/smart_ptr.h"

typedef struct aptima_data_t aptima_data_t;

aptima_RUNTIME_API aptima_shared_ptr_t *aptima_data_create(const char *name,
                                                  aptima_error_t *err);

aptima_RUNTIME_API aptima_buf_t *aptima_data_peek_buf(aptima_shared_ptr_t *self);

aptima_RUNTIME_API void aptima_data_set_buf_with_move(aptima_shared_ptr_t *self,
                                                aptima_buf_t *buf);

aptima_RUNTIME_API uint8_t *aptima_data_alloc_buf(aptima_shared_ptr_t *self,
                                            size_t size);
