//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <curl/curl.h>
#include <stdbool.h>
#include <stdint.h>

aptima_RUNTIME_PRIVATE_API bool aptima_test_curl_connect_with_retry(
    CURL *curl, uint16_t max_retries, int64_t delay_in_ms);
