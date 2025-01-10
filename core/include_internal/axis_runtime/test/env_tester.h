//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/common.h"
#include "axis_utils/lib/signature.h"

#define axis_ENV_TESTER_SIGNATURE 0x66C619FBA7DC8BD9U

typedef struct axis_extension_tester_t axis_extension_tester_t;

typedef void (*axis_env_tester_close_handler_in_target_lang_func_t)(
    void *me_in_target_lang);

typedef void (*axis_env_tester_destroy_handler_in_target_lang_func_t)(
    void *me_in_target_lang);

typedef struct axis_env_tester_t {
  axis_binding_handle_t binding_handle;

  axis_signature_t signature;
  axis_extension_tester_t *tester;

  // TODO(Wei): Do we need this close_handler?
  axis_env_tester_close_handler_in_target_lang_func_t close_handler;

  axis_env_tester_destroy_handler_in_target_lang_func_t destroy_handler;
} axis_env_tester_t;

axis_RUNTIME_API bool axis_env_tester_check_integrity(axis_env_tester_t *self);

axis_RUNTIME_PRIVATE_API axis_env_tester_t *axis_env_tester_create(
    axis_extension_tester_t *tester);

axis_RUNTIME_PRIVATE_API void axis_env_tester_destroy(axis_env_tester_t *self);

axis_RUNTIME_API void axis_env_tester_set_close_handler_in_target_lang(
    axis_env_tester_t *self,
    axis_env_tester_close_handler_in_target_lang_func_t close_handler);

axis_RUNTIME_API void axis_env_tester_set_destroy_handler_in_target_lang(
    axis_env_tester_t *self,
    axis_env_tester_destroy_handler_in_target_lang_func_t handler);
