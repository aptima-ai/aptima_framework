//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

typedef struct axis_value_t axis_value_t;
typedef struct axis_go_value_t axis_go_value_t;

void axis_go_value_finalize(axis_go_value_t *self);

/**
 * @brief Destroy the axis_value_t instance from GO.
 *
 * @param value_addr The bit pattern of the pointer to a axis_value_t. Note that
 * there is no bridge for axis_value_t.
 */
void axis_go_value_destroy(uintptr_t value_addr);

// These functions are used in getting property from axis_env_t. Refer to the
// comments in ten.h. Please keep in mind that the input axis_vale_t* is cloned
// in the previous stage (refer to axis_go_axis_property_get_type_and_size), so it
// must be destroyed in these functions.

/**
 * @param value_addr The bit pattern of the pointer to a axis_value_t. Note that
 * there is no bridge for axis_value_t.
 */
axis_go_error_t axis_go_value_get_string(uintptr_t value_addr, void *value);

axis_go_error_t axis_go_value_get_buf(uintptr_t value_addr, void *value);
