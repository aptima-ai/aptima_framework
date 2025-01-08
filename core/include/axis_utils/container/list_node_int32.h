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

#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/signature.h"

#define axis_INT32_LISTNODE_SIGNATURE 0x2A576F8836859FB5U

typedef struct axis_int32_listnode_t {
  axis_listnode_t hdr;
  axis_signature_t signature;
  int32_t int32;
} axis_int32_listnode_t;

axis_UTILS_API axis_listnode_t *axis_int32_listnode_create(int32_t int32);

axis_UTILS_API axis_int32_listnode_t *axis_listnode_to_int32_listnode(
    axis_listnode_t *self);

axis_UTILS_API axis_listnode_t *axis_listnode_from_int32_listnode(
    axis_int32_listnode_t *self);

axis_UTILS_API int32_t axis_int32_listnode_get(axis_listnode_t *self);
