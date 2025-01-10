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

axis_listnode_t *axis_list_find_ptr(axis_list_t *self, const void *ptr) {
  axis_ASSERT(self && axis_list_check_integrity(self) && ptr,
             "Invalid argument.");

  axis_list_foreach (self, iter) {
    if (axis_listnode_to_ptr_listnode(iter.node)->ptr == ptr) {
      return iter.node;
    }
  }
  return NULL;
}

axis_listnode_t *axis_list_find_ptr_custom_(axis_list_t *self, const void *ptr,
                                          bool (*equal_to)(const void *,
                                                           const void *)) {
  axis_ASSERT(self && axis_list_check_integrity(self) && ptr && equal_to,
             "Invalid argument.");

  axis_list_foreach (self, iter) {
    if (equal_to(axis_listnode_to_ptr_listnode(iter.node)->ptr, ptr)) {
      return iter.node;
    }
  }
  return NULL;
}

size_t axis_list_find_ptr_cnt_custom_(axis_list_t *self, const void *ptr,
                                     bool (*equal_to)(const void *,
                                                      const void *)) {
  axis_ASSERT(self && axis_list_check_integrity(self) && ptr && equal_to,
             "Invalid argument.");

  size_t cnt = 0;
  axis_list_foreach (self, iter) {
    if (equal_to(axis_listnode_to_ptr_listnode(iter.node)->ptr, ptr)) {
      ++cnt;
    }
  }
  return cnt;
}

size_t axis_list_cnt_ptr_custom_(axis_list_t *self,
                                bool (*predicate)(const void *)) {
  axis_ASSERT(self && axis_list_check_integrity(self) && predicate,
             "Invalid argument.");

  size_t cnt = 0;
  axis_list_foreach (self, iter) {
    if (predicate(axis_listnode_to_ptr_listnode(iter.node)->ptr)) {
      ++cnt;
    }
  }
  return cnt;
}

bool axis_list_remove_ptr(axis_list_t *self, void *ptr) {
  axis_ASSERT(self && axis_list_check_integrity(self) && ptr,
             "Invalid argument.");

  axis_list_foreach (self, iter) {
    if (axis_listnode_to_ptr_listnode(iter.node)->ptr == ptr) {
      axis_list_remove_node(self, iter.node);
      return true;
    }
  }
  return false;
}

void axis_list_push_ptr_back(axis_list_t *self, void *ptr,
                            axis_ptr_listnode_destroy_func_t destroy) {
  axis_ASSERT(self && ptr, "Invalid argument.");
  axis_listnode_t *listnode = axis_ptr_listnode_create(ptr, destroy);
  axis_list_push_back(self, listnode);
}

void axis_list_push_ptr_front(axis_list_t *self, void *ptr,
                             axis_ptr_listnode_destroy_func_t destroy) {
  axis_ASSERT(self && ptr, "Invalid argument.");
  axis_listnode_t *listnode = axis_ptr_listnode_create(ptr, destroy);
  axis_list_push_front(self, listnode);
}
