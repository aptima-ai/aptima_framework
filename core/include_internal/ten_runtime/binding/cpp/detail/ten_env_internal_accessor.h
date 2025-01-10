//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/binding/cpp/detail/ten_env.h"

namespace aptima {

class ten_env_internal_accessor_t {
 public:
  static bool init_manifest_from_json(ten_env_t &ten_env, const char *json_str,
                                      error_t *err = nullptr) {
    return ten_env.init_manifest_from_json(json_str, err);
  }

  static ::ten_env_t *get_c_ten_env(ten_env_t &ten_env) {
    return ten_env.get_c_ten_env();
  }

  static void *get_attached_target(ten_env_t &ten_env, error_t *err = nullptr) {
    return ten_env.get_attached_target(err);
  }
};

}  // namespace aptima
