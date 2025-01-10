//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/common/log.h"
#include "include_internal/axis_runtime/extension_group/builtin/builtin_extension_group.h"
#include "include_internal/axis_runtime/global/global.h"
#include "include_internal/axis_runtime/global/signal.h"
#include "include_internal/axis_utils/backtrace/backtrace.h"
#include "include_internal/axis_utils/log/log.h"
#include "axis_utils/macro/ctor.h"
#include "axis_utils/sanitizer/memory_check.h"

// LeakSanitizer checks for memory leaks when `main` ends, but functions with
// the __attribute__((destructor)) attribute are called after LeakSanitizer
// runs. Therefore, if the result of axis_MALLOC is placed into a global
// allocated memory queue used by aptima to check for memory leaks within a
// constructor function, LeakSanitizer will mistakenly report those memory
// buffers in the global allocated memory queue as memory leaks. This happens
// because these memory buffers are freed in the destructor function, but
// LeakSanitizer performs its check before that. Therefore, we should directly
// use `axis_malloc` for malloc operations within the constructor.
//
// And memory leaks within the constructor are handled by the standard ASan
// provided by Clang/GCC.
axis_CONSTRUCTOR(axis_runtime_on_load) {
  axis_sanitizer_memory_record_init();
  axis_global_signal_alt_stack_create();
  axis_backtrace_create_global();  // Initialize backtrace module.
  axis_global_init();

  axis_global_setup_signal_stuff();
  axis_log_global_init();
  axis_log_global_set_output_level(DEFAULT_LOG_OUTPUT_LEVEL);
}

axis_DESTRUCTOR(axis_runtime_on_unload) {
  axis_global_deinit();
  axis_log_global_deinit();
  axis_backtrace_destroy_global();
  axis_global_signal_alt_stack_destroy();
  axis_sanitizer_memory_record_deinit();
}
