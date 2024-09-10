//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

void ten_go_addon_unregister(uintptr_t bridge_addr);

ten_go_status_t ten_go_addon_register_extension(const void *addon_name,
                                                int addon_name_len,
                                                uintptr_t go_addon,
                                                uintptr_t *bridge_addr);

ten_go_status_t ten_go_addon_register_extension_group(const void *addon_name,
                                                      int addon_name_len,
                                                      uintptr_t go_addon,
                                                      uintptr_t *bridge_addr);
