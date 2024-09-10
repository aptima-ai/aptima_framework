//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include <stdbool.h>

#include "common.h"

typedef struct ten_go_app_t ten_go_app_t;

ten_go_app_t *ten_go_app_create(ten_go_handle_t go_app_index);

void ten_go_app_run(ten_go_app_t *app_bridge, bool run_in_background);

void ten_go_app_close(ten_go_app_t *app);

void ten_go_app_wait(ten_go_app_t *app);

// Invoked when the Go app finalizes.
void ten_go_app_finalize(ten_go_app_t *self);
