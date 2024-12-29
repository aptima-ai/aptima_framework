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

typedef struct ten_value_t ten_value_t;
typedef struct ten_go_value_t ten_go_value_t;

void ten_go_value_finalize(ten_go_value_t *self);

/**
 * @brief Destroy the ten_value_t instance from GO.
 *
 * @param value_addr The bit pattern of the pointer to a ten_value_t. Note that
 * there is no bridge for ten_value_t.
 */
void ten_go_value_destroy(uintptr_t value_addr);

// These functions are used in getting property from ten_env_t. Refer to the
// comments in ten.h. Please keep in mind that the input ten_vale_t* is cloned
// in the previous stage (refer to ten_go_ten_property_get_type_and_size), so it
// must be destroyed in these functions.

/**
 * @param value_addr The bit pattern of the pointer to a ten_value_t. Note that
 * there is no bridge for ten_value_t.
 */
ten_go_error_t ten_go_value_get_string(uintptr_t value_addr, void *value);

ten_go_error_t ten_go_value_get_buf(uintptr_t value_addr, void *value);
