//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/string.h"

TEN_RUNTIME_PRIVATE_API void ten_test_http_client_init(void);

TEN_RUNTIME_PRIVATE_API void ten_test_http_client_deinit(void);

TEN_RUNTIME_PRIVATE_API void ten_test_http_client_get(const char *url,
                                                    ten_string_t *result);

TEN_RUNTIME_PRIVATE_API void ten_test_http_client_post(const char *url,
                                                     const char *body,
                                                     ten_string_t *result);
