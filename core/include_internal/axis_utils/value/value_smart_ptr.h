//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/error.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/value_kv.h"

axis_UTILS_API bool axis_value_construct_for_smart_ptr(
    axis_value_t *self, axis_UNUSED axis_error_t *err);

axis_UTILS_API bool axis_value_copy_for_smart_ptr(axis_value_t *dest,
                                                axis_value_t *src,
                                                axis_UNUSED axis_error_t *err);

axis_UTILS_API bool axis_value_destruct_for_smart_ptr(
    axis_value_t *self, axis_UNUSED axis_error_t *err);
