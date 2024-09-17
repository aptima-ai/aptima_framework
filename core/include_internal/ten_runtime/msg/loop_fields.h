//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/error.h"
#include "ten_utils/value/value_kv.h"

typedef struct ten_msg_t ten_msg_t;

typedef bool (*ten_raw_msg_process_one_field_func_t)(ten_msg_t *msg,
                                                     ten_value_kv_t *field,
                                                     void *user_data,
                                                     ten_error_t *err);

TEN_RUNTIME_API bool ten_raw_msg_loop_all_fields(
    ten_msg_t *self, ten_raw_msg_process_one_field_func_t cb, void *user_data,
    ten_error_t *err);
