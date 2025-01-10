//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include "aptima_utils/lib/string.h"

#define aptima_PROTOCOL_TCP "tcp"
#define aptima_PROTOCOL_RAW "raw"
#define aptima_PROTOCOL_PIPE "pipe"

typedef struct aptima_value_t aptima_value_t;

aptima_UTILS_API aptima_string_t *aptima_uri_get_protocol(const char *uri);

aptima_UTILS_API bool aptima_uri_is_protocol_equal(const char *uri,
                                             const char *protocol);

aptima_UTILS_API aptima_string_t *aptima_uri_get_host(const char *uri);

aptima_UTILS_API uint16_t aptima_uri_get_port(const char *uri);
