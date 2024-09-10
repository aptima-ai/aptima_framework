//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#define MULTIPLE_APP_SCENARIO_GRAPH_CONSTRUCTION_RETRY_TIMES 3

#define CURL_CONNECT_MAX_RETRY_TIMES 10
#define CURL_CONNECT_DELAY_IN_MS 50

#define SEQUENTIAL_CLIENT_CNT 100

#if defined(__i386__)
  #define ONE_ENGINE_ONE_CLIENT_CONCURRENT_CNT 100
  #define ONE_ENGINE_ALL_CLIENT_CONCURRENT_CNT 100
#else
  #define ONE_ENGINE_ONE_CLIENT_CONCURRENT_CNT 200
  #define ONE_ENGINE_ALL_CLIENT_CONCURRENT_CNT 700
#endif
