//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/lib/string.h"

#define axis_PROTOCOL_TCP "tcp"
#define axis_PROTOCOL_RAW "raw"
#define axis_PROTOCOL_PIPE "pipe"

typedef struct axis_value_t axis_value_t;

axis_UTILS_API axis_string_t *axis_uri_get_protocol(const char *uri);

axis_UTILS_API bool axis_uri_is_protocol_equal(const char *uri,
                                             const char *protocol);

axis_UTILS_API axis_string_t *axis_uri_get_host(const char *uri);

axis_UTILS_API uint16_t axis_uri_get_port(const char *uri);
