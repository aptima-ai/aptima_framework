//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "axis_utils/lib/signature.h"

#define axis_MSG_LOCKED_RES_SIGNATURE 0x3A7C355DC39DD99EU

typedef struct axis_msg_t axis_msg_t;

typedef enum axis_MSG_LOCKED_RES_TYPE {
  axis_MSG_LOCKED_RES_TYPE_BUF,
} axis_MSG_LOCKED_RES_TYPE;

typedef struct axis_msg_locked_res_t {
  axis_signature_t signature;

  axis_MSG_LOCKED_RES_TYPE type;
} axis_msg_locked_res_t;

typedef struct axis_msg_locked_res_buf_t {
  axis_msg_locked_res_t base;
  const uint8_t *data;
} axis_msg_locked_res_buf_t;
