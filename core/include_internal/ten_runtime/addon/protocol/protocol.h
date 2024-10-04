//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/addon/addon.h"

typedef struct ten_addon_host_t ten_addon_host_t;
typedef struct ten_addon_store_t ten_addon_store_t;

#define TEN_REGISTER_ADDON_AS_PROTOCOL(PROTOCOL_NAME, ADDON) \
  TEN_ADDON_REGISTER(protocol, PROTOCOL_NAME, ADDON)

TEN_RUNTIME_PRIVATE_API ten_addon_store_t *ten_protocol_get_store(void);

TEN_RUNTIME_PRIVATE_API ten_addon_host_t *ten_addon_protocol_find(
    const char *protocol);

TEN_RUNTIME_API void ten_addon_register_protocol(const char *name,
                                                 const char *base_dir,
                                                 ten_addon_t *addon);

TEN_RUNTIME_API ten_addon_t *ten_addon_unregister_protocol(const char *name);
