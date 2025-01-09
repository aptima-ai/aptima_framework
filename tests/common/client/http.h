//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include "aptima_utils/lib/string.h"

aptima_RUNTIME_PRIVATE_API void aptima_test_http_client_init(void);

aptima_RUNTIME_PRIVATE_API void aptima_test_http_client_deinit(void);

aptima_RUNTIME_PRIVATE_API void aptima_test_http_client_get(const char *url,
                                                      aptima_string_t *result);

aptima_RUNTIME_PRIVATE_API void aptima_test_http_client_post(const char *url,
                                                       const char *body,
                                                       aptima_string_t *result);
