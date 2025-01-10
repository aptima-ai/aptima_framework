//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/metadata/metadata_info.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/addon/addon_host.h"
#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/base_dir.h"
#include "include_internal/axis_runtime/extension/base_dir.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_group/base_dir.h"
#include "include_internal/axis_runtime/extension_group/extension_group.h"
#include "include_internal/axis_runtime/extension_group/on_xxx.h"
#include "include_internal/axis_runtime/metadata/default/default.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "include_internal/axis_utils/log/log.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/path.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

bool axis_metadata_info_check_integrity(axis_metadata_info_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_METADATA_INFO_SIGNATURE) {
    return false;
  }
  return true;
}

axis_metadata_info_t *axis_metadata_info_create(axis_METADATA_ATTACH_TO attach_to,
                                              axis_env_t *belonging_to) {
  axis_ASSERT(attach_to != axis_METADATA_ATTACH_TO_INVALID, "Invalid argument.");
  axis_ASSERT(belonging_to && axis_env_check_integrity(belonging_to, true),
             "Should not happen.");
  axis_ASSERT(axis_env_get_attach_to(belonging_to) != axis_ENV_ATTACH_TO_INVALID,
             "Invalid argument.");

  axis_metadata_info_t *self =
      (axis_metadata_info_t *)axis_MALLOC(sizeof(axis_metadata_info_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_METADATA_INFO_SIGNATURE);

  self->attach_to = attach_to;
  self->type = axis_METADATA_INVALID;
  self->value = NULL;
  self->belonging_to = belonging_to;

  return self;
}

void axis_metadata_info_destroy(axis_metadata_info_t *self) {
  axis_ASSERT(self && axis_metadata_info_check_integrity(self),
             "Should not happen.");

  axis_signature_set(&self->signature, 0);

  if (self->value) {
    axis_string_destroy(self->value);
  }

  self->belonging_to = NULL;

  axis_FREE(self);
}

static axis_string_t *axis_metadata_info_filename_to_absolute_path(
    axis_metadata_info_t *self, const char *value) {
  axis_ASSERT(self && value && self->belonging_to, "Invalid argument.");

  axis_string_t filename;
  axis_string_init_formatted(&filename, "%s", value);

  bool is_absolute = axis_path_is_absolute(&filename);
  axis_string_deinit(&filename);

  if (is_absolute) {
    if (axis_path_exists(value)) {
      return axis_string_create_formatted("%s", value);
    } else {
      axis_LOGW("File '%s' does not exist.", value);
      return NULL;
    }
  }

  axis_string_t *path = NULL;
  switch (axis_env_get_attach_to(self->belonging_to)) {
    case axis_ENV_ATTACH_TO_APP: {
      const char *base_dir =
          axis_app_get_base_dir(axis_env_get_attached_app(self->belonging_to));
      if (base_dir) {
        path = axis_string_create_from_c_str(base_dir, strlen(base_dir));
      }
      break;
    }

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP: {
      const char *base_dir = axis_extension_group_get_base_dir(
          axis_env_get_attached_extension_group(self->belonging_to));
      if (base_dir) {
        path = axis_string_create_from_c_str(base_dir, strlen(base_dir));
      }
      break;
    }

    case axis_ENV_ATTACH_TO_EXTENSION: {
      const char *base_dir = axis_extension_get_base_dir(
          axis_env_get_attached_extension(self->belonging_to));
      if (base_dir) {
        path = axis_string_create_from_c_str(base_dir, strlen(base_dir));
      }
      break;
    }

    case axis_ENV_ATTACH_TO_ADDON: {
      const char *base_dir = axis_addon_host_get_base_dir(
          axis_env_get_attached_addon(self->belonging_to));
      if (base_dir) {
        path = axis_string_create_from_c_str(base_dir, strlen(base_dir));
      }
      break;
    }

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  if (!path) {
    return NULL;
  }

  int rc = axis_path_join_c_str(path, value);
  if (rc) {
    axis_LOGW("Failed to join path under %s.", axis_string_get_raw_str(path));
    axis_string_destroy(path);
    return NULL;
  }

  if (!axis_path_exists(axis_string_get_raw_str(path))) {
    axis_LOGW("File '%s' does not exist.", axis_string_get_raw_str(path));
    axis_string_destroy(path);
    return NULL;
  }

  return path;
}

static void axis_metadata_info_get_debug_display(axis_metadata_info_t *self,
                                                axis_string_t *display) {
  axis_ASSERT(
      self && axis_metadata_info_check_integrity(self) && self->belonging_to,
      "Invalid argument.");
  axis_ASSERT(display && axis_string_check_integrity(display),
             "Invalid argument.");

  switch (axis_env_get_attach_to(self->belonging_to)) {
    case axis_ENV_ATTACH_TO_ADDON:
      axis_string_set_formatted(
          display, "addon(%s)",
          axis_addon_host_get_name(
              axis_env_get_attached_addon(self->belonging_to)));
      break;

    case axis_ENV_ATTACH_TO_APP: {
      const char *uri =
          axis_app_get_uri(axis_env_get_attached_app(self->belonging_to));
      axis_string_set_formatted(display, "app(%s)", uri);
      break;
    }

    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
      axis_string_set_formatted(
          display, "extension_group(%s)",
          axis_extension_group_get_name(
              axis_env_get_attached_extension_group(self->belonging_to), true));
      break;

    case axis_ENV_ATTACH_TO_EXTENSION:
      axis_string_set_formatted(
          display, "extension(%s)",
          axis_extension_get_name(
              axis_env_get_attached_extension(self->belonging_to), true));
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }
}

bool axis_metadata_info_set(axis_metadata_info_t *self, axis_METADATA_TYPE type,
                           const char *value) {
  axis_ASSERT(self && value, "Should not happen.");

  axis_string_t display;
  axis_string_init(&display);
  axis_metadata_info_get_debug_display(self, &display);

  bool validated = false;
  axis_string_t *absolute_path = NULL;

  axis_error_t err;
  axis_error_init(&err);

  do {
    if (!value) {
      axis_LOGW("Failed to set metadata for %s, the `value` is required.",
               axis_string_get_raw_str(&display));
      break;
    }

    if (type == axis_METADATA_JSON_FILENAME) {
      absolute_path = axis_metadata_info_filename_to_absolute_path(self, value);
      if (!absolute_path) {
        axis_LOGW("Failed to set metadata for %s, file '%s' is not found.",
                 axis_string_get_raw_str(&display), value);
        break;
      }

      value = axis_string_get_raw_str(absolute_path);
    }

    switch (self->attach_to) {
      case axis_METADATA_ATTACH_TO_MANIFEST:
        switch (type) {
          case axis_METADATA_JSON_STR:
            if (self->belonging_to->attach_to == axis_ENV_ATTACH_TO_ADDON) {
              // TODO(Wei): The current protocol's manifest doesn't fully comply
              // with the spec, so we'll bypass the validation of the protocol
              // manifest for now.
              validated = true;
            } else {
              validated = axis_manifest_json_string_is_valid(value, &err);
            }
            break;

          case axis_METADATA_JSON_FILENAME:
            validated = axis_manifest_json_file_is_valid(value, &err);
            break;

          default:
            axis_ASSERT(0, "Handle more types.");
            break;
        }
        break;

      case axis_METADATA_ATTACH_TO_PROPERTY:
        switch (type) {
          case axis_METADATA_JSON_STR:
            validated = axis_property_json_string_is_valid(value, &err);
            break;

          case axis_METADATA_JSON_FILENAME:
            validated = axis_property_json_file_is_valid(value, &err);
            break;

          default:
            axis_ASSERT(0, "Handle more types.");
            break;
        }
        break;

      default:
        axis_ASSERT(0, "Should not happen.");
        break;
    }

    if (!validated) {
      axis_LOGW("Failed to set metadata for %s, %s.",
               axis_string_get_raw_str(&display), axis_error_errmsg(&err));
      break;
    }

    self->type = type;
    if (self->value) {
      axis_string_destroy(self->value);
      self->value = NULL;
    }

    self->value = axis_string_create_formatted("%s", value);
  } while (0);

  axis_string_deinit(&display);
  axis_error_deinit(&err);
  if (absolute_path) {
    axis_string_destroy(absolute_path);
  }

  return validated;
}

bool axis_handle_manifest_info_when_on_configure_done(axis_metadata_info_t **self,
                                                     const char *base_dir,
                                                     axis_value_t *manifest,
                                                     axis_error_t *err) {
  axis_ASSERT(self && *self && axis_metadata_info_check_integrity(*self),
             "Invalid argument.");
  axis_ASSERT(manifest, "Invalid argument.");

  switch (axis_env_get_attach_to((*self)->belonging_to)) {
    case axis_ENV_ATTACH_TO_APP:
    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
    case axis_ENV_ATTACH_TO_EXTENSION:
      if ((*self)->type == axis_METADATA_INVALID) {
        axis_set_default_manifest_info(base_dir, (*self), err);
      }
      break;

    case axis_ENV_ATTACH_TO_ADDON:
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  if (!axis_metadata_load_from_info(manifest, (*self), err)) {
    return false;
  }

  axis_metadata_info_destroy((*self));
  *self = NULL;

  return true;
}

bool axis_handle_property_info_when_on_configure_done(axis_metadata_info_t **self,
                                                     const char *base_dir,
                                                     axis_value_t *property,
                                                     axis_error_t *err) {
  axis_ASSERT(self && *self && axis_metadata_info_check_integrity(*self),
             "Invalid argument.");
  axis_ASSERT(property, "Invalid argument.");

  switch (axis_env_get_attach_to((*self)->belonging_to)) {
    case axis_ENV_ATTACH_TO_APP:
    case axis_ENV_ATTACH_TO_EXTENSION_GROUP:
    case axis_ENV_ATTACH_TO_EXTENSION:
      if ((*self)->type == axis_METADATA_INVALID) {
        axis_set_default_property_info(base_dir, (*self), NULL);
      }
      break;

    case axis_ENV_ATTACH_TO_ADDON:
      break;

    default:
      axis_ASSERT(0, "Should not happen.");
      break;
  }

  if (!axis_metadata_load_from_info(property, (*self), err)) {
    return false;
  }

  axis_metadata_info_destroy((*self));
  *self = NULL;

  return true;
}
