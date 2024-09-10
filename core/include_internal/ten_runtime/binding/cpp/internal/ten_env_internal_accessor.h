//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/binding/cpp/internal/ten_env.h"

namespace ten {

class ten_env_internal_accessor_t {
 public:
  explicit ten_env_internal_accessor_t(ten_env_t *ten_env) : ten_env(ten_env) {}
  ~ten_env_internal_accessor_t() = default;

  // @{
  ten_env_internal_accessor_t(const ten_env_internal_accessor_t &) = delete;
  ten_env_internal_accessor_t(ten_env_internal_accessor_t &&) = delete;
  ten_env_internal_accessor_t &operator=(const ten_env_internal_accessor_t &) =
      delete;
  ten_env_internal_accessor_t &operator=(ten_env_internal_accessor_t &&) =
      delete;
  // @}

  bool init_manifest_from_json(const char *json_str, error_t *err) {
    return ten_env->init_manifest_from_json(json_str, err);
  }

  bool init_manifest_from_json(const char *json_str) {
    return ten_env->init_manifest_from_json(json_str, nullptr);
  }

 private:
  ten_env_t *ten_env;
};

}  // namespace ten
