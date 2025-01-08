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

#define axis_GO_axis_ENV_TESTER_SIGNATURE 0x9159C741BA4A16D3U

void axis_go_axis_env_tester_finalize(uintptr_t bridge_addr);

void axis_go_axis_env_tester_on_start_done(uintptr_t bridge_addr);

axis_go_error_t axis_go_axis_env_tester_send_cmd(uintptr_t bridge_addr,
                                              uintptr_t cmd_bridge_addr,
                                              axis_go_handle_t handler_id);
