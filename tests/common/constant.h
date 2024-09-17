//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
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
