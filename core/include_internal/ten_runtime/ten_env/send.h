//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/ten_env/internal/send.h"

typedef struct ten_cmd_result_handler_for_send_cmd_ctx_t {
  ten_env_cmd_result_handler_func_t result_handler;
  void *result_handler_user_data;
} ten_cmd_result_handler_for_send_cmd_ctx_t;
