//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_utils/lib/json.h"

axis_RUNTIME_PRIVATE_API axis_json_t *axis_go_json_loads(const void *json_bytes,
                                                      int json_bytes_len,
                                                      axis_go_error_t *status);
