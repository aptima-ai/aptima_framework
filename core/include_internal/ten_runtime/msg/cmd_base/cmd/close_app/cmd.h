//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/msg/cmd_base/cmd/cmd.h"

typedef struct ten_cmd_close_app_t {
  ten_cmd_t cmd_hdr;
} ten_cmd_close_app_t;

TEN_RUNTIME_PRIVATE_API bool ten_raw_cmd_close_app_as_msg_init_from_json(
    ten_msg_t *self, ten_json_t *json, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_msg_t *
ten_raw_cmd_close_app_as_msg_create_from_json(ten_json_t *json,
                                              ten_error_t *err);

TEN_RUNTIME_PRIVATE_API void ten_raw_cmd_close_app_as_msg_destroy(
    ten_msg_t *self);

TEN_RUNTIME_PRIVATE_API ten_json_t *ten_raw_cmd_close_app_to_json(
    ten_msg_t *self, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_cmd_close_app_t *ten_raw_cmd_close_app_create(void);
