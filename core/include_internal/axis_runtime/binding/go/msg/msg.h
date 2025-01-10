//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "src/axis_runtime/binding/go/interface/aptima/msg.h"
#include "axis_utils/lib/signature.h"

#define axis_GO_MSG_SIGNATURE 0xB0E144BC5D3B1AB9U

typedef struct axis_go_msg_t {
  axis_signature_t signature;

  axis_shared_ptr_t *c_msg;
  axis_go_handle_t go_msg;
} axis_go_msg_t;

axis_RUNTIME_PRIVATE_API axis_go_msg_t *axis_go_msg_reinterpret(uintptr_t msg);

axis_RUNTIME_PRIVATE_API bool axis_go_msg_check_integrity(axis_go_msg_t *self);

axis_RUNTIME_PRIVATE_API axis_go_handle_t
axis_go_msg_go_handle(axis_go_msg_t *self);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_go_msg_c_msg(axis_go_msg_t *self);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_go_msg_move_c_msg(
    axis_go_msg_t *self);

axis_RUNTIME_PRIVATE_API axis_go_msg_t *axis_go_msg_create(
    axis_shared_ptr_t *c_msg);

axis_RUNTIME_PRIVATE_API void axis_go_msg_set_go_handle(
    axis_go_msg_t *self, axis_go_handle_t go_handle);
