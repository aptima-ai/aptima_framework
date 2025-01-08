//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "axis_utils/container/list.h"
#include "axis_utils/lib/buf.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/value_kv.h"

#define axis_VALUE_KV_SIGNATURE 0xCF7DC27C3B187517U

axis_UTILS_API axis_value_kv_t *axis_value_kv_create_vempty(const char *fmt, ...);
