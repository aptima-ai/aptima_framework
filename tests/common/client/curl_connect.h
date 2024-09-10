//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <curl/curl.h>
#include <stdbool.h>
#include <stdint.h>

TEN_RUNTIME_PRIVATE_API bool ten_test_curl_connect_with_retry(
    CURL *curl, uint16_t max_retries, int64_t delay_in_ms);
