//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/lib/error.h"

namespace ten {

class error_t {
 public:
  explicit error_t() : c_error(aptima_error_create()), own_(true) {}

  explicit error_t(aptima_error_t *err, bool own = true)
      : c_error(err), own_(own) {}

  ~error_t() {
    if (own_) {
      aptima_error_destroy(c_error);
    }
  };

  error_t(const error_t &) = delete;
  error_t &operator=(const error_t &) = delete;

  error_t(error_t &&) = delete;
  error_t &operator=(error_t &&) = delete;

  const char *err_msg() { return aptima_error_errmsg(c_error); }

  // Internal use only.
  bool is_success() { return aptima_error_is_success(c_error); }
  aptima_error_t *get_c_error() { return c_error; }

 private:
  aptima_error_t *c_error;
  bool own_;
};

}  // namespace ten
