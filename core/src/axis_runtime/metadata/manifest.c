//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/metadata/manifest.h"

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "axis_utils/lib/file.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/memory.h"

bool axis_manifest_get_type_and_name(const char *filename, axis_ADDON_TYPE *type,
                                    axis_string_t *name, axis_error_t *err) {
  axis_ASSERT(type, "Invalid argument.");
  axis_ASSERT(name, "Invalid argument.");

  if (!filename || strlen(filename) == 0) {
    axis_LOGW("Try to load manifest but file name not provided");
    return false;
  }

  char *buf = axis_file_read(filename);
  if (!buf) {
    axis_LOGW("Can not read content from %s", filename);
    return false;
  }

  axis_json_t *json = axis_json_from_string(buf, err);
  axis_FREE(buf);
  if (!json) {
    return false;
  }

  const char *type_str = axis_json_object_peek_string(json, axis_STR_TYPE);
  *type = axis_addon_type_from_string(type_str);

  const char *name_str = axis_json_object_peek_string(json, axis_STR_NAME);
  axis_string_set_from_c_str(name, name_str, strlen(name_str));

  axis_json_destroy(json);

  return true;
}
