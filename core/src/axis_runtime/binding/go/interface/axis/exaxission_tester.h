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

axis_go_error_t axis_go_extension_tester_create(
    axis_go_handle_t go_extension_tester, uintptr_t *bridge_addr);

void axis_go_extension_tester_finalize(uintptr_t bridge_addr);
