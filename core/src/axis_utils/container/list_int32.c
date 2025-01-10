//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/macro/check.h"

axis_listnode_t *axis_list_find_int32(axis_list_t *self, int32_t int32) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");

  axis_list_foreach (self, iter) {
    int32_t self_value = axis_listnode_to_int32_listnode(iter.node)->int32;
    if (self_value == int32) {
      return iter.node;
    }
  }
  return NULL;
}
