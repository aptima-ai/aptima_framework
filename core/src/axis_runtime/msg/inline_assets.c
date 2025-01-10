//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/msg.h"

extern inline bool axis_raw_msg_is_cmd_and_result(axis_msg_t *self);  // NOLINT

extern inline bool axis_raw_msg_is_cmd(axis_msg_t *self);  // NOLINT

extern inline bool axis_raw_msg_is_cmd_result(axis_msg_t *self);  // NOLINT

extern inline axis_msg_t *axis_msg_get_raw_msg(axis_shared_ptr_t *self);  // NOLINT

extern inline bool axis_msg_is_cmd_and_result(axis_shared_ptr_t *self);  // NOLINT

extern inline bool axis_msg_is_cmd(axis_shared_ptr_t *self);  // NOLINT

extern inline bool axis_msg_is_cmd_result(axis_shared_ptr_t *self);  // NOLINT

extern inline axis_MSG_TYPE axis_raw_msg_get_type(axis_msg_t *self);  // NOLINT
