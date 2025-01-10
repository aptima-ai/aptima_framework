//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/container/list.h"

axis_RUNTIME_API int axis_py_is_initialized(void);

axis_RUNTIME_API void axis_py_initialize(void);

axis_RUNTIME_API int axis_py_finalize(void);

axis_RUNTIME_API void axis_py_add_paths_to_sys(axis_list_t *paths);

axis_RUNTIME_API void axis_py_run_simple_string(const char *code);

axis_RUNTIME_API const char *axis_py_get_path(void);

axis_RUNTIME_API void axis_py_mem_free(void *ptr);

axis_RUNTIME_API void axis_py_import_module(const char *module_name);

axis_RUNTIME_API void *axis_py_eval_save_thread(void);

axis_RUNTIME_API void axis_py_eval_restore_thread(void *state);

// @{
// To prevent callers from needing to include Python's header files, which would
// require `-I` and `-L` command-line parameters, the two functions below
// deliberately use `void *` as the parameter type for passing arguments.
axis_RUNTIME_API void *axis_py_gil_state_ensure(void);

axis_RUNTIME_API void axis_py_gil_state_release(void *state);
// @}
