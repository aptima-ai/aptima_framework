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
#include "axis_utils/container/list_node_smart_ptr.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"

axis_listnode_t *axis_list_find_shared_ptr_custom_(
    axis_list_t *self, const void *ptr,
    bool (*equal_to)(const void *, const void *)) {
  axis_ASSERT(self && axis_list_check_integrity(self) && equal_to,
             "Invalid argument.");

  axis_list_foreach (self, iter) {
    if (equal_to(axis_shared_ptr_get_data(
                     axis_listnode_to_smart_ptr_listnode(iter.node)->ptr),
                 ptr)) {
      return iter.node;
    }
  }
  return NULL;
}

axis_listnode_t *axis_list_find_shared_ptr_custom_2_(
    axis_list_t *self, const void *ptr_1, const void *ptr_2,
    bool (*equal_to)(const void *, const void *, const void *)) {
  axis_ASSERT(self && axis_list_check_integrity(self) && equal_to,
             "Invalid argument.");

  axis_list_foreach (self, iter) {
    if (equal_to(axis_shared_ptr_get_data(
                     axis_listnode_to_smart_ptr_listnode(iter.node)->ptr),
                 ptr_1, ptr_2)) {
      return iter.node;
    }
  }
  return NULL;
}

axis_listnode_t *axis_list_find_shared_ptr_custom_3_(
    axis_list_t *self, const void *ptr_1, const void *ptr_2, const void *ptr_3,
    bool (*equal_to)(const void *, const void *, const void *, const void *)) {
  axis_ASSERT(self && axis_list_check_integrity(self) && equal_to,
             "Invalid argument.");

  axis_list_foreach (self, iter) {
    if (equal_to(axis_shared_ptr_get_data(
                     axis_listnode_to_smart_ptr_listnode(iter.node)->ptr),
                 ptr_1, ptr_2, ptr_3)) {
      return iter.node;
    }
  }
  return NULL;
}

axis_listnode_t *axis_list_find_shared_ptr_custom_4_(
    axis_list_t *self, const void *ptr_1, const void *ptr_2, const void *ptr_3,
    const void *ptr_4,
    bool (*equal_to)(const void *, const void *, const void *, const void *,
                     const void *)) {
  axis_ASSERT(self && axis_list_check_integrity(self) && equal_to,
             "Invalid argument.");

  axis_list_foreach (self, iter) {
    if (equal_to(axis_shared_ptr_get_data(
                     axis_listnode_to_smart_ptr_listnode(iter.node)->ptr),
                 ptr_1, ptr_2, ptr_3, ptr_4)) {
      return iter.node;
    }
  }
  return NULL;
}

axis_smart_ptr_t *axis_list_push_smart_ptr_back(axis_list_t *self,
                                              axis_smart_ptr_t *ptr) {
  axis_ASSERT(self && axis_list_check_integrity(self) && ptr,
             "Invalid argument.");

  axis_listnode_t *listnode = axis_smart_ptr_listnode_create(ptr);
  axis_list_push_back(self, listnode);
  return ((axis_smart_ptr_listnode_t *)listnode)->ptr;
}

static bool axis_list_ptr_equal_to(const void *ptr_in_list,
                                  const void *raw_ptr) {
  axis_ASSERT(ptr_in_list && raw_ptr, "Invalid argument.");

  return ptr_in_list == raw_ptr;
}

axis_listnode_t *axis_list_find_shared_ptr(axis_list_t *self, const void *ptr) {
  axis_ASSERT(self && axis_list_check_integrity(self) && ptr,
             "Invalid argument.");

  return axis_list_find_shared_ptr_custom_(self, ptr, axis_list_ptr_equal_to);
}
