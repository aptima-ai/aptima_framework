//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_runtime/common/status_code.h"
#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/smart_ptr.h"

typedef struct aptima_cmd_result_t aptima_cmd_result_t;

aptima_RUNTIME_API aptima_shared_ptr_t *aptima_cmd_result_create(
    aptima_STATUS_CODE status_code);

aptima_RUNTIME_API aptima_shared_ptr_t *aptima_cmd_result_create_from_cmd(
    aptima_STATUS_CODE status_code, aptima_shared_ptr_t *original_cmd);

aptima_RUNTIME_API aptima_STATUS_CODE
aptima_cmd_result_get_status_code(aptima_shared_ptr_t *self);

aptima_RUNTIME_API bool aptima_cmd_result_is_final(aptima_shared_ptr_t *self,
                                             aptima_error_t *err);

aptima_RUNTIME_API bool aptima_cmd_result_is_completed(aptima_shared_ptr_t *self,
                                                 aptima_error_t *err);

aptima_RUNTIME_API bool aptima_cmd_result_set_final(aptima_shared_ptr_t *self,
                                              bool is_final, aptima_error_t *err);
