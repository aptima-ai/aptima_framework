//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "ten_utils/lib/signature.h"

#define TEN_MSG_LOCKED_RES_SIGNATURE 0x3A7C355DC39DD99EU

typedef struct ten_msg_t ten_msg_t;

typedef enum TEN_MSG_LOCKED_RES_TYPE {
  TEN_MSG_LOCKED_RES_TYPE_BUF,
} TEN_MSG_LOCKED_RES_TYPE;

typedef struct ten_msg_locked_res_t {
  ten_signature_t signature;

  TEN_MSG_LOCKED_RES_TYPE type;
} ten_msg_locked_res_t;

typedef struct ten_msg_locked_res_buf_t {
  ten_msg_locked_res_t base;
  const uint8_t *data;
} ten_msg_locked_res_buf_t;
