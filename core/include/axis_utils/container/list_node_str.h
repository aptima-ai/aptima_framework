//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stddef.h>

#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"

#define axis_STR_LISTNODE_SIGNATURE 0x15D33B50C27A1B20U

typedef struct axis_str_listnode_t {
  axis_listnode_t hdr;
  axis_signature_t signature;
  axis_string_t str;
} axis_str_listnode_t;

axis_UTILS_API axis_listnode_t *axis_str_listnode_create(const char *str);

axis_UTILS_API axis_listnode_t *axis_str_listnode_create_with_size(const char *str,
                                                                size_t size);

axis_UTILS_API axis_str_listnode_t *axis_listnode_to_str_listnode(
    axis_listnode_t *self);

axis_UTILS_API axis_listnode_t *axis_listnode_from_str_listnode(
    axis_str_listnode_t *self);

axis_UTILS_API axis_string_t *axis_str_listnode_get(axis_listnode_t *self);
