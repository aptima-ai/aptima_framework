//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_config.h"

// TODO(Wei): Note that this function has no effect on windows now.
axis_RUNTIME_API void axis_global_setup_signal_stuff(void);

axis_RUNTIME_PRIVATE_API void axis_global_signal_alt_stack_create(void);

axis_RUNTIME_PRIVATE_API void axis_global_signal_alt_stack_destroy(void);
