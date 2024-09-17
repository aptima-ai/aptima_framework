//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/metadata/metadata.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/schema_store/store.h"
#include "include_internal/ten_rust/ten_rust.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/file.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_is.h"
#include "ten_utils/value/value_object.h"

static bool ten_metadata_load_from_json_string(ten_value_t *metadata,
                                               const char *json_str,
                                               ten_error_t *err) {
  TEN_ASSERT(metadata && ten_value_check_integrity(metadata) && json_str,
             "Should not happen.");

  ten_json_t *json = ten_json_from_string(json_str, NULL);
  if (!json) {
    return false;
  }

  bool rc = ten_value_object_merge_with_json(metadata, json);

  ten_json_destroy(json);

  return rc;
}

static bool ten_metadata_load_from_json_file(ten_value_t *metadata,
                                             const char *filename,
                                             ten_error_t *err) {
  TEN_ASSERT(metadata && ten_value_check_integrity(metadata),
             "Should not happen.");

  if (!filename || strlen(filename) == 0) {
    TEN_LOGW("Try to load metadata but file name not provided");
    return false;
  }

  char *buf = ten_file_read(filename);
  if (!buf) {
    TEN_LOGW("Can not read content from %s", filename);
    return false;
  }

  bool ret = ten_metadata_load_from_json_string(metadata, buf, err);
  if (!ret) {
    TEN_LOGW(
        "Try to load metadata from file '%s', but file content with wrong "
        "format",
        filename);
  }

  TEN_FREE(buf);

  return ret;
}

static bool ten_metadata_load_from_type_ane_value(ten_value_t *metadata,
                                                  TEN_METADATA_TYPE type,
                                                  const char *value,
                                                  ten_error_t *err) {
  TEN_ASSERT(metadata, "Invalid argument.");

  bool result = true;

  switch (type) {
    case TEN_METADATA_INVALID:
      break;

    case TEN_METADATA_JSON_STR:
      if (!ten_metadata_load_from_json_string(metadata, value, err)) {
        result = false;
        goto done;
      }
      break;

    case TEN_METADATA_JSON_FILENAME:
      if (!ten_metadata_load_from_json_file(metadata, value, err)) {
        result = false;
        goto done;
      }
      break;

    default:
      TEN_ASSERT(0 && "Handle more types.", "Should not happen.");
      break;
  }

done:
  return result;
}

bool ten_metadata_load_from_info(ten_value_t *metadata,
                                 ten_metadata_info_t *metadata_info,
                                 ten_error_t *err) {
  TEN_ASSERT(metadata && metadata_info, "Invalid argument.");

  return ten_metadata_load_from_type_ane_value(
      metadata, metadata_info->type,
      metadata_info->value ? ten_string_get_raw_str(metadata_info->value) : "",
      err);
}

void ten_metadata_load(ten_object_on_init_func_t on_init, ten_env_t *ten_env) {
  TEN_ASSERT(on_init && ten_env && ten_env_check_integrity(ten_env, true),
             "Should not happen.");

  on_init(ten_env);
}

ten_value_t *ten_metadata_init_schema_store(ten_value_t *manifest,
                                            ten_schema_store_t *schema_store) {
  TEN_ASSERT(manifest && ten_value_check_integrity(manifest),
             "Invalid argument.");
  TEN_ASSERT(ten_value_is_object(manifest), "Should not happen.");
  TEN_ASSERT(schema_store, "Invalid argument.");

  ten_value_t *api_definition = ten_value_object_peek(manifest, TEN_STR_API);
  if (!api_definition) {
    return NULL;
  }

  ten_error_t err;
  ten_error_init(&err);
  if (!ten_schema_store_set_schema_definition(schema_store, api_definition,
                                              &err)) {
    TEN_LOGW("Failed to set schema definition: %s.", ten_error_errmsg(&err));
  }

  ten_error_deinit(&err);

  return api_definition;
}

bool ten_manifest_json_string_is_valid(const char *json_string,
                                       ten_error_t *err) {
  TEN_ASSERT(json_string, "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  const char *err_msg = NULL;
  bool rc = ten_validate_manifest_json_string(json_string, &err_msg);

  if (!rc) {
    ten_error_set(err, TEN_ERRNO_GENERIC, err_msg);
    ten_rust_free_cstring(err_msg);
    return false;
  }

  return true;
}

bool ten_manifest_json_file_is_valid(const char *json_file, ten_error_t *err) {
  TEN_ASSERT(json_file, "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  const char *err_msg = NULL;
  bool rc = ten_validate_manifest_json_file(json_file, &err_msg);

  if (!rc) {
    ten_error_set(err, TEN_ERRNO_GENERIC, err_msg);
    ten_rust_free_cstring(err_msg);
    return false;
  }

  return true;
}

bool ten_property_json_string_is_valid(const char *json_string,
                                       ten_error_t *err) {
  TEN_ASSERT(json_string, "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  const char *err_msg = NULL;
  bool rc = ten_validate_property_json_string(json_string, &err_msg);

  if (!rc) {
    ten_error_set(err, TEN_ERRNO_GENERIC, err_msg);
    ten_rust_free_cstring(err_msg);
    return false;
  }

  return true;
}

bool ten_property_json_file_is_valid(const char *json_file, ten_error_t *err) {
  TEN_ASSERT(json_file, "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  const char *err_msg = NULL;
  bool rc = ten_validate_property_json_file(json_file, &err_msg);

  if (!rc) {
    ten_error_set(err, TEN_ERRNO_GENERIC, err_msg);
    ten_rust_free_cstring(err_msg);
    return false;
  }

  return true;
}
