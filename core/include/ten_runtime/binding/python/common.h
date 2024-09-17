//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/container/list.h"

TEN_RUNTIME_API int ten_py_is_initialized(void);

TEN_RUNTIME_API void ten_py_initialize(void);

TEN_RUNTIME_API int ten_py_finalize(void);

TEN_RUNTIME_API void ten_py_add_paths_to_sys(ten_list_t *paths);

TEN_RUNTIME_API void ten_py_run_simple_string(const char *code);

TEN_RUNTIME_API const char *ten_py_get_path(void);

TEN_RUNTIME_API void ten_py_mem_free(void *ptr);

TEN_RUNTIME_API void ten_py_import_module(const char *module_name);

TEN_RUNTIME_API void *ten_py_eval_save_thread(void);

TEN_RUNTIME_API void ten_py_eval_restore_thread(void *state);
