//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

namespace ten {

class binding_handle_t {
 protected:
  explicit binding_handle_t(void *c_instance) : _c_instance(c_instance) {
    TEN_LOGE("2 %p", c_instance);
    TEN_ASSERT(c_instance, "Should not happen.");
  }

  void set_c_instance(void *c_instance) { this->_c_instance = c_instance; }

 public:
  void *get_c_instance() const {
    TEN_LOGE("3 %p", _c_instance);
    return _c_instance;
  }

 private:
  void *_c_instance = nullptr;
};

}  // namespace ten
