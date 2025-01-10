//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/data/field/buf.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/msg/data/data.h"
#include "include_internal/axis_runtime/msg/msg.h"

bool axis_data_process_buf(axis_msg_t *self,
                          axis_raw_msg_process_one_field_func_t cb,
                          void *user_data, axis_error_t *err) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Should not happen.");

  axis_msg_field_process_data_t buf_field;
  axis_msg_field_process_data_init(&buf_field, axis_STR_BUF,
                                  &((axis_data_t *)self)->buf, false);

  bool rc = cb(self, &buf_field, user_data, err);

  return rc;
}
