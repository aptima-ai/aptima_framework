//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <curl/curl.h>
#include <stdbool.h>
#include <stdint.h>

axis_RUNTIME_PRIVATE_API bool axis_test_curl_connect_with_retry(
    CURL *curl, uint16_t max_retries, int64_t delay_in_ms);
