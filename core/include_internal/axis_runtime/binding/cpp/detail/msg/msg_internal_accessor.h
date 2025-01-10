//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <memory>

#include "axis_runtime/binding/cpp/detail/msg/msg.h"

namespace aptima {

class msg_internal_accessor_t {
 public:
  explicit msg_internal_accessor_t() = default;
  ~msg_internal_accessor_t() = default;

  // @{
  msg_internal_accessor_t(const msg_internal_accessor_t &) = delete;
  msg_internal_accessor_t(msg_internal_accessor_t &&) = delete;
  msg_internal_accessor_t &operator=(const msg_internal_accessor_t &) = delete;
  msg_internal_accessor_t &operator=(msg_internal_accessor_t &&) = delete;
  // @}

  static axis_MSG_TYPE get_type(const msg_t *msg, error_t *err = nullptr) {
    return msg->get_type(err);
  }
};

}  // namespace aptima
