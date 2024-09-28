//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/ten_env/ten_env.h"

/**
 * @brief Indicate that extension on_start/on_stop/on_deinit is completed.
 */
typedef struct ten_extension_on_start_stop_deinit_done_t {
  // Indicates which extension's on_start/on_stop/on_deinit ends.
  ten_extension_t *extension;
} ten_extension_on_start_stop_deinit_done_t;

TEN_RUNTIME_PRIVATE_API ten_extension_on_start_stop_deinit_done_t *
ten_extension_on_start_stop_deinit_done_create(ten_extension_t *extension);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_start_stop_deinit_done_destroy(
    ten_extension_on_start_stop_deinit_done_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_configure_done(ten_env_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_init_done(ten_env_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_start_done(ten_env_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_stop_done(ten_env_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_deinit_done(ten_env_t *self);
