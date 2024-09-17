//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stdbool.h>

#include "ten_utils/ten_config.h"
#include "ten_utils/value/value.h"

TEN_UTILS_API bool ten_value_set_int64(ten_value_t *self, int64_t value);

TEN_UTILS_API bool ten_value_set_int32(ten_value_t *self, int32_t value);

TEN_UTILS_API bool ten_value_set_int16(ten_value_t *self, int16_t value);

TEN_UTILS_API bool ten_value_set_int8(ten_value_t *self, int8_t value);

TEN_UTILS_API bool ten_value_set_uint64(ten_value_t *self, uint64_t value);

TEN_UTILS_API bool ten_value_set_uint32(ten_value_t *self, uint32_t value);

TEN_UTILS_API bool ten_value_set_uint16(ten_value_t *self, uint16_t value);

TEN_UTILS_API bool ten_value_set_uint8(ten_value_t *self, uint8_t value);

TEN_UTILS_API bool ten_value_set_bool(ten_value_t *self, bool value);
