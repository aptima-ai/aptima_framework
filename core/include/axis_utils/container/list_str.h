//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>
#include <stddef.h>

#include "aptima_utils/container/list_node.h"

aptima_UTILS_API void aptima_list_push_str_back(aptima_list_t *self, const char *str);

aptima_UTILS_API void aptima_list_push_str_front(aptima_list_t *self, const char *str);

aptima_UTILS_API void aptima_list_push_str_with_size_back(aptima_list_t *self,
                                                    const char *str,
                                                    size_t size);

aptima_UTILS_API aptima_listnode_t *aptima_list_find_string(aptima_list_t *self,
                                                   const char *str);
