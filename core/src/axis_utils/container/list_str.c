//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/macro/check.h"

void axis_list_push_str_back(axis_list_t *self, const char *str) {
  axis_ASSERT(self && axis_list_check_integrity(self) && str,
             "Invalid argument.");

  axis_listnode_t *listnode = axis_str_listnode_create(str);
  axis_list_push_back(self, listnode);
}

void axis_list_push_str_front(axis_list_t *self, const char *str) {
  axis_ASSERT(self && str, "Invalid argument.");

  axis_listnode_t *listnode = axis_str_listnode_create(str);
  axis_list_push_front(self, listnode);
}

void axis_list_push_str_with_size_back(axis_list_t *self, const char *str,
                                      size_t size) {
  axis_ASSERT(self && axis_list_check_integrity(self) && str,
             "Invalid argument.");

  axis_listnode_t *listnode = axis_str_listnode_create_with_size(str, size);
  axis_list_push_back(self, listnode);
}

axis_listnode_t *axis_list_find_string(axis_list_t *self, const char *str) {
  axis_ASSERT(self && axis_list_check_integrity(self) && str,
             "Invalid argument.");

  axis_list_foreach (self, iter) {
    if (axis_string_is_equal_c_str(
            &(axis_listnode_to_str_listnode(iter.node)->str), str)) {
      return iter.node;
    }
  }
  return NULL;
}
