//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/lib/signature.h"
#include "aptima_utils/lib/string.h"

#define aptima_LISTNODE_SIGNATURE 0x00D7B10E642B105CU

typedef struct aptima_listnode_t aptima_listnode_t;
typedef struct aptima_list_t aptima_list_t;

struct aptima_listnode_t {
  aptima_signature_t signature;
  aptima_listnode_t *next, *prev;
  void (*destroy)(aptima_listnode_t *);
};

#include "aptima_utils/container/list_node_int32.h"      // IWYU pragma: keep
#include "aptima_utils/container/list_node_ptr.h"        // IWYU pragma: keep
#include "aptima_utils/container/list_node_smart_ptr.h"  // IWYU pragma: keep
#include "aptima_utils/container/list_node_str.h"        // IWYU pragma: keep

aptima_UTILS_PRIVATE_API bool aptima_listnode_check_integrity(aptima_listnode_t *self);

aptima_UTILS_PRIVATE_API void aptima_listnode_init(aptima_listnode_t *self,
                                             void *destroy);

aptima_UTILS_API void aptima_listnode_destroy(aptima_listnode_t *self);
aptima_UTILS_API void aptima_listnode_destroy_only(aptima_listnode_t *self);
