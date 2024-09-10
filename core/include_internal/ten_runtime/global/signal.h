//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_runtime/ten_config.h"

// TODO(Wei): Note that this function has no effect on windows now.
TEN_RUNTIME_API void ten_global_setup_signal_stuff(void);

TEN_RUNTIME_PRIVATE_API void ten_global_signal_alt_stack_create(void);

TEN_RUNTIME_PRIVATE_API void ten_global_signal_alt_stack_destroy(void);
