//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stddef.h>

#include "aptima_utils/container/list_node.h"
#include "aptima_utils/lib/signature.h"
#include "aptima_utils/lib/string.h"

#define aptima_STR_LISTNODE_SIGNATURE 0x15D33B50C27A1B20U

typedef struct aptima_str_listnode_t {
  aptima_listnode_t hdr;
  aptima_signature_t signature;
  aptima_string_t str;
} aptima_str_listnode_t;

aptima_UTILS_API aptima_listnode_t *aptima_str_listnode_create(const char *str);

aptima_UTILS_API aptima_listnode_t *aptima_str_listnode_create_with_size(const char *str,
                                                                size_t size);

aptima_UTILS_API aptima_str_listnode_t *aptima_listnode_to_str_listnode(
    aptima_listnode_t *self);

aptima_UTILS_API aptima_listnode_t *aptima_listnode_from_str_listnode(
    aptima_str_listnode_t *self);

aptima_UTILS_API aptima_string_t *aptima_str_listnode_get(aptima_listnode_t *self);
