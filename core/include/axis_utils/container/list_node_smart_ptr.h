//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/container/list_node.h"
#include "aptima_utils/lib/signature.h"
#include "aptima_utils/lib/smart_ptr.h"

#define aptima_SMART_PTR_LISTNODE_SIGNATURE 0x00C0ADEEF6B9A421U

typedef struct aptima_smart_ptr_listnode_t {
  aptima_listnode_t hdr;
  aptima_signature_t signature;
  aptima_smart_ptr_t *ptr;
} aptima_smart_ptr_listnode_t;

aptima_UTILS_API aptima_listnode_t *aptima_smart_ptr_listnode_create(
    aptima_smart_ptr_t *ptr);

aptima_UTILS_API aptima_smart_ptr_listnode_t *aptima_listnode_to_smart_ptr_listnode(
    aptima_listnode_t *self);

aptima_UTILS_API aptima_listnode_t *aptima_listnode_from_smart_ptr_listnode(
    aptima_smart_ptr_listnode_t *self);

aptima_UTILS_API aptima_smart_ptr_t *aptima_smart_ptr_listnode_get(aptima_listnode_t *self);
