//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"

#define axis_SMART_PTR_LISTNODE_SIGNATURE 0x00C0ADEEF6B9A421U

typedef struct axis_smart_ptr_listnode_t {
  axis_listnode_t hdr;
  axis_signature_t signature;
  axis_smart_ptr_t *ptr;
} axis_smart_ptr_listnode_t;

axis_UTILS_API axis_listnode_t *axis_smart_ptr_listnode_create(
    axis_smart_ptr_t *ptr);

axis_UTILS_API axis_smart_ptr_listnode_t *axis_listnode_to_smart_ptr_listnode(
    axis_listnode_t *self);

axis_UTILS_API axis_listnode_t *axis_listnode_from_smart_ptr_listnode(
    axis_smart_ptr_listnode_t *self);

axis_UTILS_API axis_smart_ptr_t *axis_smart_ptr_listnode_get(axis_listnode_t *self);
