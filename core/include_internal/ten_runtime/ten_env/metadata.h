//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/metadata/metadata.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/ten_env/ten_env.h"

typedef enum TEN_METADATA_LEVEL {
  TEN_METADATA_LEVEL_INVALID,

  TEN_METADATA_LEVEL_APP,
  TEN_METADATA_LEVEL_EXTENSION_GROUP,
  TEN_METADATA_LEVEL_EXTENSION,
  TEN_METADATA_LEVEL_ADDON,
} TEN_METADATA_LEVEL;

TEN_RUNTIME_PRIVATE_API TEN_METADATA_LEVEL ten_determine_metadata_level(
    TEN_ENV_ATTACH_TO attach_to_type, const char **p_name);

TEN_RUNTIME_API bool ten_env_init_manifest_from_json(ten_env_t *self,
                                                     const char *json_string,
                                                     ten_error_t *err);
