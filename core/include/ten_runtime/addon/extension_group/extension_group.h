//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

typedef struct ten_addon_t ten_addon_t;
typedef struct ten_addon_host_t ten_addon_host_t;

#define TEN_REGISTER_ADDON_AS_EXTENSION_GROUP(NAME, ADDON) \
  TEN_ADDON_REGISTER(extension_group, NAME, ADDON)

TEN_RUNTIME_API ten_addon_host_t *ten_addon_register_extension_group(
    const char *name, ten_addon_t *addon);

TEN_RUNTIME_API void ten_addon_unregister_extension_group(const char *name,
                                                        ten_addon_t *addon);
