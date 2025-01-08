//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/metadata/metadata.h"

#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/schema_store/store.h"
#include "axis_runtime/axis_env/axis_env.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/file.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_object.h"

#if defined(axis_ENABLE_axis_RUST_APIS)
#include "include_internal/axis_rust/axis_rust.h"
#endif

static bool axis_metadata_load_from_json_str(axis_value_t *metadata,
                                            const char *json_str,
                                            axis_error_t *err) {
  axis_ASSERT(metadata && axis_value_check_integrity(metadata) && json_str,
             "Should not happen.");

  axis_json_t *json = axis_json_from_string(json_str, err);
  if (!json) {
    return false;
  }

  bool rc = axis_value_object_merge_with_json(metadata, json);

  axis_json_destroy(json);

  return rc;
}

static bool axis_metadata_load_from_json_file(axis_value_t *metadata,
                                             const char *filename,
                                             axis_error_t *err) {
  axis_ASSERT(metadata && axis_value_check_integrity(metadata),
             "Should not happen.");

  if (!filename || strlen(filename) == 0) {
    axis_LOGW("Try to load metadata but file name not provided");
    return false;
  }

  char *buf = axis_file_read(filename);
  if (!buf) {
    axis_LOGW("Can not read content from %s", filename);
    return false;
  }

  bool ret = axis_metadata_load_from_json_str(metadata, buf, err);
  if (!ret) {
    axis_LOGW(
        "Try to load metadata from file '%s', but file content with wrong "
        "format",
        filename);
  }

  axis_FREE(buf);

  return ret;
}

static bool axis_metadata_load_from_type_ane_value(axis_value_t *metadata,
                                                  axis_METADATA_TYPE type,
                                                  const char *value,
                                                  axis_error_t *err) {
  axis_ASSERT(metadata, "Invalid argument.");

  bool result = true;

  switch (type) {
    case axis_METADATA_INVALID:
      break;

    case axis_METADATA_JSON_STR:
      if (!axis_metadata_load_from_json_str(metadata, value, err)) {
        result = false;
        goto done;
      }
      break;

    case axis_METADATA_JSON_FILENAME:
      if (!axis_metadata_load_from_json_file(metadata, value, err)) {
        result = false;
        goto done;
      }
      break;

    default:
      axis_ASSERT(0 && "Handle more types.", "Should not happen.");
      break;
  }

done:
  return result;
}

bool axis_metadata_load_from_info(axis_value_t *metadata,
                                 axis_metadata_info_t *metadata_info,
                                 axis_error_t *err) {
  axis_ASSERT(metadata && metadata_info, "Invalid argument.");

  return axis_metadata_load_from_type_ane_value(
      metadata, metadata_info->type,
      metadata_info->value ? axis_string_get_raw_str(metadata_info->value) : "",
      err);
}

void axis_metadata_load(axis_object_on_configure_func_t on_configure,
                       axis_env_t *axis_env) {
  axis_ASSERT(on_configure && axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");

  on_configure(axis_env);
}

axis_value_t *axis_metadata_init_schema_store(axis_value_t *manifest,
                                            axis_schema_store_t *schema_store) {
  axis_ASSERT(manifest && axis_value_check_integrity(manifest),
             "Invalid argument.");
  axis_ASSERT(axis_value_is_object(manifest), "Should not happen.");
  axis_ASSERT(schema_store, "Invalid argument.");

  axis_value_t *api_definition = axis_value_object_peek(manifest, axis_STR_API);
  if (!api_definition) {
    return NULL;
  }

  axis_error_t err;
  axis_error_init(&err);
  if (!axis_schema_store_set_schema_definition(schema_store, api_definition,
                                              &err)) {
    axis_LOGW("Failed to set schema definition: %s.", axis_error_errmsg(&err));
  }

  axis_error_deinit(&err);

  return api_definition;
}

bool axis_manifest_json_string_is_valid(const char *json_string,
                                       axis_error_t *err) {
  axis_ASSERT(json_string, "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

#if defined(axis_ENABLE_axis_RUST_APIS)
  const char *err_msg = NULL;
  bool rc = axis_validate_manifest_json_string(json_string, &err_msg);

  if (!rc) {
    axis_error_set(err, axis_ERRNO_GENERIC, err_msg);
    axis_rust_free_cstring(err_msg);
    return false;
  }
#endif

  return true;
}

bool axis_manifest_json_file_is_valid(const char *json_file, axis_error_t *err) {
  axis_ASSERT(json_file, "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

#if defined(axis_ENABLE_axis_RUST_APIS)
  const char *err_msg = NULL;
  bool rc = axis_validate_manifest_json_file(json_file, &err_msg);

  if (!rc) {
    axis_error_set(err, axis_ERRNO_GENERIC, err_msg);
    axis_rust_free_cstring(err_msg);
    return false;
  }
#endif

  return true;
}

bool axis_property_json_string_is_valid(const char *json_string,
                                       axis_error_t *err) {
  axis_ASSERT(json_string, "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

#if defined(axis_ENABLE_axis_RUST_APIS)
  const char *err_msg = NULL;
  bool rc = axis_validate_property_json_string(json_string, &err_msg);

  if (!rc) {
    axis_error_set(err, axis_ERRNO_GENERIC, err_msg);
    axis_rust_free_cstring(err_msg);
    return false;
  }
#endif

  return true;
}

bool axis_property_json_file_is_valid(const char *json_file, axis_error_t *err) {
  axis_ASSERT(json_file, "Invalid argument.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

#if defined(axis_ENABLE_axis_RUST_APIS)
  const char *err_msg = NULL;
  bool rc = axis_validate_property_json_file(json_file, &err_msg);

  if (!rc) {
    axis_error_set(err, axis_ERRNO_GENERIC, err_msg);
    axis_rust_free_cstring(err_msg);
    return false;
  }
#endif

  return true;
}
