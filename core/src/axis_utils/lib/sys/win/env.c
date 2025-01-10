//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/lib/env.h"

#include <stdlib.h>

bool axis_env_set(const char *name, const char *value) {
  if (_putenv_s(name, value) != 0) {
    return false;
  }

  return true;
}
