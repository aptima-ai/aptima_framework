//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/msg/loop_fields.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"

#define TEN_DATA_SIGNATURE 0xC579EAD75E1BB0FCU

typedef struct ten_data_t {
  ten_msg_t msg_hdr;
  ten_signature_t signature;
  ten_value_t buf;
} ten_data_t;

TEN_RUNTIME_PRIVATE_API bool ten_raw_data_like_set_ten_property(
    ten_msg_t *self, ten_list_t *paths, ten_value_t *value, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_raw_data_check_integrity(ten_data_t *self);

TEN_RUNTIME_PRIVATE_API void ten_raw_data_buf_copy(
    ten_msg_t *self, ten_msg_t *src, ten_list_t *excluded_field_ids);

TEN_RUNTIME_PRIVATE_API ten_msg_t *ten_raw_data_as_msg_clone(
    ten_msg_t *self, ten_list_t *excluded_field_ids);

TEN_RUNTIME_PRIVATE_API ten_msg_t *ten_raw_data_as_msg_create_from_json(
    ten_json_t *json, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_raw_data_as_msg_init_from_json(
    ten_msg_t *self, ten_json_t *json, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_json_t *ten_raw_data_as_msg_to_json(
    ten_msg_t *self, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_raw_data_loop_all_fields(
    ten_msg_t *self, ten_raw_msg_process_one_field_func_t cb, void *user_data,
    ten_error_t *err);

TEN_RUNTIME_PRIVATE_API void ten_raw_data_destroy(ten_data_t *self);

TEN_RUNTIME_PRIVATE_API bool ten_raw_data_check_type_and_name(
    ten_msg_t *self, const char *type_str, const char *name_str,
    ten_error_t *err);
