//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

ten_go_status_t ten_go_extension_group_create(
    ten_go_handle_t go_extension_group_index, const void *name, int name_len,
    uintptr_t *bridge_addr);

// Invoked when the Go extension group finalizes.
void ten_go_extension_group_finalize(uintptr_t bridge_addr);
