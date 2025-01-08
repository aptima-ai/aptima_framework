//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stddef.h>

#include "axis_utils/container/list_node.h"

axis_UTILS_API void axis_list_push_str_back(axis_list_t *self, const char *str);

axis_UTILS_API void axis_list_push_str_front(axis_list_t *self, const char *str);

axis_UTILS_API void axis_list_push_str_with_size_back(axis_list_t *self,
                                                    const char *str,
                                                    size_t size);

axis_UTILS_API axis_listnode_t *axis_list_find_string(axis_list_t *self,
                                                   const char *str);
