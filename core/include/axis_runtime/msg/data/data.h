//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stddef.h>

#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_data_t axis_data_t;

axis_RUNTIME_API axis_shared_ptr_t *axis_data_create(const char *name,
                                                  axis_error_t *err);

axis_RUNTIME_API axis_buf_t *axis_data_peek_buf(axis_shared_ptr_t *self);

axis_RUNTIME_API void axis_data_set_buf_with_move(axis_shared_ptr_t *self,
                                                axis_buf_t *buf);

axis_RUNTIME_API uint8_t *axis_data_alloc_buf(axis_shared_ptr_t *self,
                                            size_t size);
