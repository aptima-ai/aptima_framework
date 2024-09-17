//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stdbool.h>

TEN_UTILS_PRIVATE_API bool can_convert_float64_to_int8(double a);

TEN_UTILS_PRIVATE_API bool can_convert_float64_to_int16(double a);

TEN_UTILS_PRIVATE_API bool can_convert_float64_to_int32(double a);

TEN_UTILS_PRIVATE_API bool can_convert_float64_to_int64(double a);

TEN_UTILS_PRIVATE_API bool can_convert_float64_to_uint8(double a);

TEN_UTILS_PRIVATE_API bool can_convert_float64_to_uint16(double a);

TEN_UTILS_PRIVATE_API bool can_convert_float64_to_uint32(double a);

TEN_UTILS_PRIVATE_API bool can_convert_float64_to_uint64(double a);

TEN_UTILS_PRIVATE_API bool can_convert_int32_to_float32(int32_t value);

TEN_UTILS_PRIVATE_API bool can_convert_int64_to_float32(int64_t value);

TEN_UTILS_PRIVATE_API bool can_convert_uint32_to_float32(uint32_t value);

TEN_UTILS_PRIVATE_API bool can_convert_uint64_to_float32(uint64_t value);

TEN_UTILS_PRIVATE_API bool can_convert_int32_to_float64(int32_t value);

TEN_UTILS_PRIVATE_API bool can_convert_int64_to_float64(int64_t value);

TEN_UTILS_PRIVATE_API bool can_convert_uint32_to_float64(uint32_t value);

TEN_UTILS_PRIVATE_API bool can_convert_uint64_to_float64(uint64_t value);
