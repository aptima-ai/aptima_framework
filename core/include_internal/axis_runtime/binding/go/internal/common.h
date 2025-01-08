//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stddef.h>

#include "src/axis_runtime/binding/go/interface/ten/common.h"
#include "axis_utils/lib/error.h"

#define axis_GO_STATUS_ERR_MSG_BUF_SIZE 256

axis_RUNTIME_PRIVATE_API axis_go_handle_array_t *axis_go_handle_array_create(
    size_t size);

axis_RUNTIME_PRIVATE_API void axis_go_handle_array_destroy(
    axis_go_handle_array_t *self);

axis_RUNTIME_PRIVATE_API char *axis_go_str_dup(const char *str);

axis_RUNTIME_PRIVATE_API void axis_go_bridge_destroy_c_part(
    axis_go_bridge_t *self);

axis_RUNTIME_PRIVATE_API void axis_go_bridge_destroy_go_part(
    axis_go_bridge_t *self);

axis_RUNTIME_PRIVATE_API void axis_go_error_init_with_errno(axis_go_error_t *self,
                                                          axis_errno_t err_no);

axis_RUNTIME_PRIVATE_API void axis_go_error_from_error(axis_go_error_t *self,
                                                     axis_error_t *err);

axis_RUNTIME_PRIVATE_API void axis_go_error_set_errno(axis_go_error_t *self,
                                                    axis_errno_t err_no);

axis_RUNTIME_PRIVATE_API void axis_go_error_set(axis_go_error_t *self,
                                              axis_errno_t err_no,
                                              const char *msg);
