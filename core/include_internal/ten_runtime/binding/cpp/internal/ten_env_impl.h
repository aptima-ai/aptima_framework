//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <string>

#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "include_internal/ten_runtime/ten_env/metadata.h"

namespace ten {

inline bool ten_env_t::init_manifest_from_json(const char *json, error_t *err) {
  TEN_ASSERT(c_ten_env, "Should not happen.");

  if (json == nullptr) {
    TEN_ASSERT(0, "Invalid argument.");
    return false;
  }

  return ten_env_init_manifest_from_json(
      c_ten_env, json,
      err != nullptr ? err->get_internal_representation() : nullptr);
}

}  // namespace ten
