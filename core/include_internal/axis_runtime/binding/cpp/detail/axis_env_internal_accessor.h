//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/binding/cpp/detail/axis_env.h"

namespace aptima {

class axis_env_internal_accessor_t {
 public:
  static bool init_manifest_from_json(axis_env_t &axis_env, const char *json_str,
                                      error_t *err = nullptr) {
    return axis_env.init_manifest_from_json(json_str, err);
  }

  static ::axis_env_t *get_c_axis_env(axis_env_t &axis_env) {
    return axis_env.get_c_axis_env();
  }

  static void *get_attached_target(axis_env_t &axis_env, error_t *err = nullptr) {
    return axis_env.get_attached_target(err);
  }
};

}  // namespace aptima
