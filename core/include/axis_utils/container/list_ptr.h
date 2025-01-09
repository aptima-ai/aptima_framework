//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "aptima_utils/container/list_node.h"

#define aptima_list_find_ptr_custom(self, ptr, equal_to)  \
  aptima_list_find_ptr_custom_(self, (const void *)(ptr), \
                            (bool (*)(const void *, const void *))(equal_to));

aptima_UTILS_API aptima_listnode_t *aptima_list_find_ptr_custom_(
    aptima_list_t *self, const void *ptr,
    bool (*equal_to)(const void *, const void *));

#define aptima_list_find_ptr_cnt_custom(self, ptr, equal_to) \
  aptima_list_find_ptr_cnt_custom_(                          \
      self, (const void *)(ptr),                          \
      (bool (*)(const void *, const void *))(equal_to));

aptima_UTILS_API size_t
aptima_list_find_ptr_cnt_custom_(aptima_list_t *self, const void *ptr,
                              bool (*equal_to)(const void *, const void *));

#define aptima_list_cnt_ptr_custom(self, predicate) \
  aptima_list_cnt_ptr_custom_(self, (bool (*)(const void *))(predicate));

aptima_UTILS_API size_t aptima_list_cnt_ptr_custom_(aptima_list_t *self,
                                              bool (*predicate)(const void *));

aptima_UTILS_API aptima_listnode_t *aptima_list_find_ptr(aptima_list_t *self,
                                                const void *ptr);

aptima_UTILS_API bool aptima_list_remove_ptr(aptima_list_t *self, void *ptr);

aptima_UTILS_API void aptima_list_push_ptr_back(
    aptima_list_t *self, void *ptr, aptima_ptr_listnode_destroy_func_t destroy);

aptima_UTILS_API void aptima_list_push_ptr_front(
    aptima_list_t *self, void *ptr, aptima_ptr_listnode_destroy_func_t destroy);
