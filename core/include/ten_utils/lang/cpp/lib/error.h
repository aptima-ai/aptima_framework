//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/lib/error.h"

namespace ten {

class error_t {
 public:
  explicit error_t() : c_error(ten_error_create()), own_(true) {}

  explicit error_t(ten_error_t *err, bool own = true)
      : c_error(err), own_(own) {}

  ~error_t() {
    if (own_) {
      ten_error_destroy(c_error);
    }
  };

  error_t(const error_t &) = delete;
  error_t &operator=(const error_t &) = delete;

  error_t(error_t &&) = delete;
  error_t &operator=(error_t &&) = delete;

  void reset() { ten_error_reset(c_error); }

  bool is_success() { return ten_error_is_success(c_error); }

  const char *errmsg() { return ten_error_errmsg(c_error); }

  // Internal use only.
  ten_error_t *get_internal_representation() { return c_error; }

 private:
  ten_error_t *c_error;
  bool own_;
};

}  // namespace ten
