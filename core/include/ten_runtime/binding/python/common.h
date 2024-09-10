//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
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
