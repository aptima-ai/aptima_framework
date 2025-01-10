//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "aptima_utils/container/list_node.h"
#include "aptima_utils/lib/signature.h"

#define aptima_INT32_LISTNODE_SIGNATURE 0x2A576F8836859FB5U

typedef struct aptima_int32_listnode_t {
  aptima_listnode_t hdr;
  aptima_signature_t signature;
  int32_t int32;
} aptima_int32_listnode_t;

aptima_UTILS_API aptima_listnode_t *aptima_int32_listnode_create(int32_t int32);

aptima_UTILS_API aptima_int32_listnode_t *aptima_listnode_to_int32_listnode(
    aptima_listnode_t *self);

aptima_UTILS_API aptima_listnode_t *aptima_listnode_from_int32_listnode(
    aptima_int32_listnode_t *self);

aptima_UTILS_API int32_t aptima_int32_listnode_get(aptima_listnode_t *self);
