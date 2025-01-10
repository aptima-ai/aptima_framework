//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/error.h"

typedef struct axis_metadata_info_t axis_metadata_info_t;

axis_RUNTIME_PRIVATE_API void axis_set_default_manifest_info(
    const char *base_dir, axis_metadata_info_t *manifest, axis_error_t *err);

axis_RUNTIME_PRIVATE_API void axis_set_default_property_info(
    const char *base_dir, axis_metadata_info_t *property, axis_error_t *err);
