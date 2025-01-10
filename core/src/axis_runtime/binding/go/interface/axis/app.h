//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stdbool.h>

#include "common.h"

typedef struct axis_go_app_t axis_go_app_t;

axis_go_app_t *axis_go_app_create(axis_go_handle_t go_app_index);

void axis_go_app_run(axis_go_app_t *app_bridge, bool run_in_background);

void axis_go_app_close(axis_go_app_t *app);

void axis_go_app_wait(axis_go_app_t *app);

// Invoked when the Go app finalizes.
void axis_go_app_finalize(axis_go_app_t *self);
