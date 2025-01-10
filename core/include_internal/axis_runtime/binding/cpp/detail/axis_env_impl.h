//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "include_internal/axis_runtime/axis_env/metadata.h"
#include "axis_runtime/binding/cpp/detail/axis_env.h"

namespace aptima {

inline bool axis_env_t::init_manifest_from_json(const char *json, error_t *err) {
  axis_ASSERT(c_axis_env, "Should not happen.");

  if (json == nullptr) {
    axis_ASSERT(0, "Invalid argument.");
    return false;
  }

  return axis_env_init_manifest_from_json(
      c_axis_env, json, err != nullptr ? err->get_c_error() : nullptr);
}

}  // namespace aptima
