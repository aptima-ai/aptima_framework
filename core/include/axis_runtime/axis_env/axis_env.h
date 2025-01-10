//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_runtime/common/errno.h"                  // IWYU pragma: keep
#include "aptima_runtime/aptima_env/internal/log.h"          // IWYU pragma: keep
#include "aptima_runtime/aptima_env/internal/metadata.h"     // IWYU pragma: keep
#include "aptima_runtime/aptima_env/internal/on_xxx_done.h"  // IWYU pragma: keep
#include "aptima_runtime/aptima_env/internal/return.h"       // IWYU pragma: keep
#include "aptima_runtime/aptima_env/internal/send.h"         // IWYU pragma: keep

typedef struct aptima_env_t aptima_env_t;
typedef struct aptima_extension_group_t aptima_extension_group_t;

aptima_RUNTIME_API bool aptima_env_check_integrity(aptima_env_t *self,
                                             bool check_thread);

aptima_RUNTIME_API void *aptima_env_get_attached_target(aptima_env_t *self);

aptima_RUNTIME_API void aptima_env_destroy(aptima_env_t *self);
