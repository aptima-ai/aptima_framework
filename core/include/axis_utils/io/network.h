//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "aptima_utils/lib/string.h"

#define IPV4_STR_MAX_LEN (15 + 1)  // 1 for the null terminator
#define IPV6_STR_MAX_LEN (39 + 1)  // 1 for the null terminator
#define IP_STR_MAX_LEN IPV6_STR_MAX_LEN
#define URI_MAX_LEN 256
#define PORT_MIN_NUM 1
#define PORT_MAX_NUM 65535

// FIXME(Ender): buggy interface
// Have to handle multiple interface cases
aptima_UTILS_API void aptima_host_get(char *hostname_buffer,
                                size_t hostname_buffer_capacity,
                                char *ip_buffer, size_t ip_buffer_capacity);

aptima_UTILS_API bool aptima_host_split(const char *uri, char *name_buffer,
                                  size_t name_buffer_capacity, int32_t *port);

aptima_UTILS_API bool aptima_get_ipv6_prefix(const char *ifid, aptima_string_t *prefix);
