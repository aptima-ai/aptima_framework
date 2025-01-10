//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

#include <stdint.h>

#include "gtest/gtest.h"
#include "aptima_utils/lib/base64.h"
#include "aptima_utils/lib/buf.h"

TEST(Base64Test, positive) {
  aptima_string_t result;
  aptima_string_init(&result);

  const char *src_str = "how_are_you_this_morning";
  aptima_buf_t src_str_buf = aptima_BUF_STATIC_INIT_WITH_DATA_UNOWNED(
      (uint8_t *)src_str, strlen(src_str));
  bool rc = aptima_base64_to_string(&result, &src_str_buf);
  EXPECT_EQ(rc, true);
  EXPECT_EQ(
      aptima_string_is_equal_c_str(&result, "aG93X2FyZV95b3VfdGhpc19tb3JuaW5n"),
      true);

  aptima_buf_deinit(&src_str_buf);

  aptima_buf_t convert_back_data = aptima_BUF_STATIC_INIT_OWNED;
  rc = aptima_base64_from_string(&result, &convert_back_data);
  EXPECT_EQ(rc, true);
  EXPECT_NE(convert_back_data.content_size, 0);
  EXPECT_EQ(aptima_c_string_is_equal_with_size(
                src_str, (const char *)convert_back_data.data,
                convert_back_data.content_size),
            true);

  aptima_buf_deinit(&convert_back_data);
  aptima_string_deinit(&result);
}
