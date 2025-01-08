//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/common/base_dir.h"

#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "axis_utils/lib/string.h"

const char *axis_extension_get_base_dir(axis_extension_t *self) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Invalid argument.");

  if (self->addon_host) {
    return axis_string_get_raw_str(&self->addon_host->base_dir);
  } else {
    return NULL;
  }
}
