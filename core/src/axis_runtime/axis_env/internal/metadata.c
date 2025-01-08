//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_env/internal/metadata.h"

#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/axis_env/metadata.h"
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/axis_env/metadata.h"
#include "include_internal/axis_runtime/extension_context/extension_context.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/axis_env/metadata.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/axis_env/metadata.h"
#include "include_internal/axis_runtime/axis_env/metadata_cb.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

static axis_METADATA_LEVEL axis_get_metadata_level_from_name(
    const char **p_name, axis_METADATA_LEVEL default_level) {
  axis_ASSERT(p_name, "Invalid argument.");

  axis_METADATA_LEVEL metadata_level = default_level;

  const char *name = *p_name;
  if (!name || strlen(name) == 0) {
    return metadata_level;
  }

  const char *delimiter_position =
      strstr(name, axis_STR_PROPERTY_STORE_SCOPE_DELIMITER);
  if (delimiter_position != NULL) {
    // Determine which level of the property store where the current property
    // should be accessed.
    if (axis_c_string_starts_with(
            name, axis_STR_EXTENSION axis_STR_PROPERTY_STORE_SCOPE_DELIMITER)) {
      metadata_level = axis_METADATA_LEVEL_EXTENSION;
      *p_name +=
          strlen(axis_STR_EXTENSION axis_STR_PROPERTY_STORE_SCOPE_DELIMITER);
    } else if (axis_c_string_starts_with(
                   name, axis_STR_EXTENSION_GROUP
                             axis_STR_PROPERTY_STORE_SCOPE_DELIMITER)) {
      metadata_level = axis_METADATA_LEVEL_EXTENSION_GROUP;
      *p_name += strlen(
          axis_STR_EXTENSION_GROUP axis_STR_PROPERTY_STORE_SCOPE_DELIMITER);
    } else if (axis_c_string_starts_with(
                   name, axis_STR_APP axis_STR_PROPERTY_STORE_SCOPE_DELIMITER)) {
      metadata_level = axis_METADATA_LEVEL_APP;
      *p_name += strlen(axis_STR_APP axis_STR_PROPERTY_STORE_SCOPE_DELIMITER);
    } else {
      int prefix_length = (int)(delimiter_position - name);
      axis_LOGE("Unknown level: %.*s", prefix_length, name);
    }
  }

  return metadata_level;
}

axis_METADATA_LEVEL axis_determine_metadata_level(
    axis_ENV_ATTACH_TO attach_to_type, const char **p_name) {
  axis_ASSERT(p_name, "Invalid argument.");

  axis_METADATA_LEVEL level = axis_METADATA_LEVEL_INVALID;

  switch (attach_to_type) {
    case axis_ENV_ATTACH_TO_EXTENSION:
      level = axis_get_metadata_level_from_name(p_name,
                                               axis_METADATA_LEVEL_EXTENSION);
      break;

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
      level = axis_get_metadata_level_from_name(
          p_name, axis_METADATA_LEVEL_EXTENSION_GROUP);
      axis_ASSERT(level != axis_METADATA_LEVEL_EXTENSION, "Should not happen.");
      break;

    case axis_ENV_ATTACH_TO_APP:
      level = axis_get_metadata_level_from_name(p_name, axis_METADATA_LEVEL_APP);
      axis_ASSERT(level == axis_METADATA_LEVEL_APP, "Should not happen.");
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return level;
}

bool axis_env_is_property_exist(axis_env_t *self, const char *path,
                               axis_error_t *err) {
  axis_ASSERT(self && axis_env_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(path && strlen(path), "path should not be empty.");

  if (!path || !strlen(path)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                    "path should not be empty.");
    }
    return false;
  }

  // If the property cannot be found, it should not be an error, just return
  // false.
  axis_value_t *value = axis_env_peek_property(self, path, NULL);
  if (value != NULL) {
    return true;
  } else {
    return false;
  }
}

static axis_metadata_info_t *axis_env_get_manifest_info(axis_env_t *self) {
  axis_metadata_info_t *manifest_info = NULL;

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION:
      manifest_info = self->attached_target.extension->manifest_info;
      break;

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
      manifest_info = self->attached_target.extension_group->manifest_info;
      break;

    case axis_ENV_ATTACH_TO_APP:
      manifest_info = self->attached_target.app->manifest_info;
      break;

    case axis_ENV_ATTACH_TO_ADDON:
      manifest_info = self->attached_target.addon_host->manifest_info;
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return manifest_info;
}

static axis_metadata_info_t *axis_env_get_property_info(axis_env_t *self) {
  axis_metadata_info_t *property_info = NULL;

  switch (self->attach_to) {
    case axis_ENV_ATTACH_TO_EXTENSION:
      property_info = self->attached_target.extension->property_info;
      break;

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
      property_info = self->attached_target.extension_group->property_info;
      break;

    case axis_ENV_ATTACH_TO_APP:
      property_info = self->attached_target.app->property_info;
      break;

    case axis_ENV_ATTACH_TO_ADDON:
      property_info = self->attached_target.addon_host->property_info;
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  return property_info;
}

bool axis_env_init_manifest_from_json(axis_env_t *self, const char *json_string,
                                     axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(
      axis_env_check_integrity(
          self, self->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Invalid use of axis_env %p.", self);

  axis_metadata_info_t *manifest_info = axis_env_get_manifest_info(self);
  axis_ASSERT(manifest_info && axis_metadata_info_check_integrity(manifest_info),
             "Should not happen.");

  return axis_metadata_info_set(manifest_info, axis_METADATA_JSON_STR,
                               json_string);
}

bool axis_env_init_property_from_json(axis_env_t *self, const char *json_string,
                                     axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(
      axis_env_check_integrity(
          self, self->attach_to != axis_ENV_ATTACH_TO_ADDON ? true : false),
      "Invalid use of axis_env %p.", self);

  axis_metadata_info_t *property_info = axis_env_get_property_info(self);
  axis_ASSERT(property_info && axis_metadata_info_check_integrity(property_info),
             "Should not happen.");

  return axis_metadata_info_set(property_info, axis_METADATA_JSON_STR,
                               json_string);
}
