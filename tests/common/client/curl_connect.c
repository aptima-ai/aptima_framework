//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "tests/common/client/curl_connect.h"

#include "aptima_utils/lib/time.h"
#include "aptima_utils/log/log.h"

bool aptima_test_curl_connect_with_retry(CURL *curl, uint16_t max_retries,
                                      int64_t delay_in_ms) {
  CURLcode res = CURLE_OK;

  int count = 0;
  do {
    res = curl_easy_perform(curl);

    // 'CURLE_GOT_NOTHING' means the client has connected to the server, but the
    // server replies nothing.
    if (res == CURLE_OK || res == CURLE_GOT_NOTHING) {
      return true;
    }

    aptima_sleep(delay_in_ms);
  } while (++count < max_retries);

  aptima_LOGE("Failed to connect to server, error code: %d.", res);

  return false;
}
