//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "axis_utils/container/list_node.h"

#define axis_list_find_ptr_custom(self, ptr, equal_to)  \
  axis_list_find_ptr_custom_(self, (const void *)(ptr), \
                            (bool (*)(const void *, const void *))(equal_to));

axis_UTILS_API axis_listnode_t *axis_list_find_ptr_custom_(
    axis_list_t *self, const void *ptr,
    bool (*equal_to)(const void *, const void *));

#define axis_list_find_ptr_cnt_custom(self, ptr, equal_to) \
  axis_list_find_ptr_cnt_custom_(                          \
      self, (const void *)(ptr),                          \
      (bool (*)(const void *, const void *))(equal_to));

axis_UTILS_API size_t
axis_list_find_ptr_cnt_custom_(axis_list_t *self, const void *ptr,
                              bool (*equal_to)(const void *, const void *));

#define axis_list_cnt_ptr_custom(self, predicate) \
  axis_list_cnt_ptr_custom_(self, (bool (*)(const void *))(predicate));

axis_UTILS_API size_t axis_list_cnt_ptr_custom_(axis_list_t *self,
                                              bool (*predicate)(const void *));

axis_UTILS_API axis_listnode_t *axis_list_find_ptr(axis_list_t *self,
                                                const void *ptr);

axis_UTILS_API bool axis_list_remove_ptr(axis_list_t *self, void *ptr);

axis_UTILS_API void axis_list_push_ptr_back(
    axis_list_t *self, void *ptr, axis_ptr_listnode_destroy_func_t destroy);

axis_UTILS_API void axis_list_push_ptr_front(
    axis_list_t *self, void *ptr, axis_ptr_listnode_destroy_func_t destroy);
