//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/addon/addon.h"

typedef struct ten_addon_host_t ten_addon_host_t;
typedef struct ten_addon_store_t ten_addon_store_t;

#define TEN_REGISTER_ADDON_AS_LANG_ADDON_LOADER(LANG_ADDON_LOADER_NAME, ADDON) \
  TEN_ADDON_REGISTER(lang_addon_loader, LANG_ADDON_LOADER_NAME, ADDON)

TEN_RUNTIME_PRIVATE_API ten_addon_store_t *
ten_lang_addon_loader_get_global_store(void);

TEN_RUNTIME_PRIVATE_API ten_addon_host_t *ten_addon_lang_addon_loader_find(
    const char *lang_addon_loader);

TEN_RUNTIME_API ten_addon_host_t *ten_addon_register_lang_addon_loader(
    const char *name, const char *base_dir, ten_addon_t *addon,
    void *register_ctx);

TEN_RUNTIME_API ten_addon_t *ten_addon_unregister_lang_addon_loader(
    const char *name);

TEN_RUNTIME_API void ten_addon_unregister_all_lang_addon_loader(void);
