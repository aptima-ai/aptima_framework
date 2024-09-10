//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/error.h"

typedef struct ten_metadata_info_t ten_metadata_info_t;

TEN_RUNTIME_PRIVATE_API void ten_set_default_manifest_info(
    const char *base_dir, ten_metadata_info_t *manifest, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API void ten_set_default_property_info(
    const char *base_dir, ten_metadata_info_t *property, ten_error_t *err);
