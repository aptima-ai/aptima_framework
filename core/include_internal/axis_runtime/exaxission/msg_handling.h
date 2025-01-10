//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/smart_ptr.h"

typedef struct axis_extension_t axis_extension_t;
typedef struct axis_msg_conversion_t axis_msg_conversion_t;

axis_RUNTIME_PRIVATE_API void axis_extension_handle_in_msg(axis_extension_t *self,
                                                         axis_shared_ptr_t *msg);
