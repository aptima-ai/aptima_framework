//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/ten_env/ten_env.h"

extern inline ten_extension_t *ten_env_get_attached_extension(ten_env_t *self);

extern inline ten_extension_group_t *ten_env_get_attached_extension_group(
    ten_env_t *self);

extern inline ten_app_t *ten_env_get_attached_app(ten_env_t *self);

extern inline ten_addon_host_t *ten_env_get_attached_addon(ten_env_t *self);

extern inline ten_engine_t *ten_env_get_attached_engine(ten_env_t *self);
