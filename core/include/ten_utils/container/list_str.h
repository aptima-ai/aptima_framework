//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stdbool.h>
#include <stddef.h>

#include "ten_utils/container/list_node.h"

TEN_UTILS_API void ten_list_push_str_back(ten_list_t *self, const char *str);

TEN_UTILS_API void ten_list_push_str_front(ten_list_t *self, const char *str);

TEN_UTILS_API void ten_list_push_str_with_size_back(ten_list_t *self,
                                                    const char *str,
                                                    size_t size);

TEN_UTILS_API ten_listnode_t *ten_list_find_string(ten_list_t *self,
                                                   const char *str);
