//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/ten_config.h"

#include "ten_utils/macro/mark.h"

typedef struct ten_addon_t ten_addon_t;
typedef struct ten_env_t ten_env_t;

TEN_RUNTIME_PRIVATE_API void ten_builtin_extension_group_addon_on_init(
    TEN_UNUSED ten_addon_t *addon, ten_env_t *ten_env);

TEN_RUNTIME_PRIVATE_API void ten_builtin_extension_group_addon_create_instance(
    ten_addon_t *addon, ten_env_t *ten_env, const char *name, void *context);

TEN_RUNTIME_PRIVATE_API void ten_builtin_extension_group_addon_destroy_instance(
    TEN_UNUSED ten_addon_t *addon, ten_env_t *ten_env, void *_extension_group,
    void *context);

TEN_RUNTIME_PRIVATE_API void ten_builtin_extension_group_addon_register(void);

TEN_RUNTIME_PRIVATE_API void ten_builtin_extension_group_addon_unregister(void);
