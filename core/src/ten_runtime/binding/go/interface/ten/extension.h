//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

ten_go_status_t ten_go_extension_create(ten_go_handle_t go_extension,
                                        const void *name, int name_len,
                                        uintptr_t *bridge_addr);

// Invoked when the Go extension finalizes.
void ten_go_extension_finalize(uintptr_t bridge_addr);
