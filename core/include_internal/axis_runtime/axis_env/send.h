//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_runtime/axis_env/internal/send.h"

typedef struct axis_cmd_result_handler_for_send_cmd_ctx_t {
  axis_env_msg_result_handler_func_t result_handler;
  void *result_handler_user_data;
} axis_cmd_result_handler_for_send_cmd_ctx_t;
