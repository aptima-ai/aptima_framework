//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "src/axis_runtime/binding/go/interface/aptima/common.h"
#include "src/axis_runtime/binding/go/interface/aptima/metadata.h"

#define axis_GO_METADATA_SIGNATURE 0xC3D0D91971B40C4FU

axis_RUNTIME_PRIVATE_API bool axis_go_metadata_check_integrity(
    axis_go_metadata_t *self);

axis_RUNTIME_PRIVATE_API axis_go_handle_t
axis_go_metadata_go_handle(axis_go_metadata_t *self);

axis_RUNTIME_PRIVATE_API axis_metadata_info_t *axis_go_metadata_c_metadata(
    axis_go_metadata_t *self);

axis_RUNTIME_PRIVATE_API axis_go_metadata_t *axis_go_metadata_wrap(
    axis_metadata_info_t *c_metadata);
