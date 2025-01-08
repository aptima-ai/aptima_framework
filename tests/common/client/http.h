//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/lib/string.h"

axis_RUNTIME_PRIVATE_API void axis_test_http_client_init(void);

axis_RUNTIME_PRIVATE_API void axis_test_http_client_deinit(void);

axis_RUNTIME_PRIVATE_API void axis_test_http_client_get(const char *url,
                                                      axis_string_t *result);

axis_RUNTIME_PRIVATE_API void axis_test_http_client_post(const char *url,
                                                       const char *body,
                                                       axis_string_t *result);
