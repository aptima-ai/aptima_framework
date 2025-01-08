//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"

#define axis_LISTNODE_SIGNATURE 0x00D7B10E642B105CU

typedef struct axis_listnode_t axis_listnode_t;
typedef struct axis_list_t axis_list_t;

struct axis_listnode_t {
  axis_signature_t signature;
  axis_listnode_t *next, *prev;
  void (*destroy)(axis_listnode_t *);
};

#include "axis_utils/container/list_node_int32.h"      // IWYU pragma: keep
#include "axis_utils/container/list_node_ptr.h"        // IWYU pragma: keep
#include "axis_utils/container/list_node_smart_ptr.h"  // IWYU pragma: keep
#include "axis_utils/container/list_node_str.h"        // IWYU pragma: keep

axis_UTILS_PRIVATE_API bool axis_listnode_check_integrity(axis_listnode_t *self);

axis_UTILS_PRIVATE_API void axis_listnode_init(axis_listnode_t *self,
                                             void *destroy);

axis_UTILS_API void axis_listnode_destroy(axis_listnode_t *self);
axis_UTILS_API void axis_listnode_destroy_only(axis_listnode_t *self);
