//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/value/value.h"

typedef struct axis_extension_group_info_t axis_extension_group_info_t;

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_extension_group_info_from_value(
    axis_value_t *value, axis_list_t *extension_groups_info, axis_error_t *err);
