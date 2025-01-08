//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//

#include <stdint.h>

#include "gtest/gtest.h"
#include "axis_utils/lib/base64.h"
#include "axis_utils/lib/buf.h"

TEST(Base64Test, positive) {
  axis_string_t result;
  axis_string_init(&result);

  const char *src_str = "how_are_you_this_morning";
  axis_buf_t src_str_buf = axis_BUF_STATIC_INIT_WITH_DATA_UNOWNED(
      (uint8_t *)src_str, strlen(src_str));
  bool rc = axis_base64_to_string(&result, &src_str_buf);
  EXPECT_EQ(rc, true);
  EXPECT_EQ(
      axis_string_is_equal_c_str(&result, "aG93X2FyZV95b3VfdGhpc19tb3JuaW5n"),
      true);

  axis_buf_deinit(&src_str_buf);

  axis_buf_t convert_back_data = axis_BUF_STATIC_INIT_OWNED;
  rc = axis_base64_from_string(&result, &convert_back_data);
  EXPECT_EQ(rc, true);
  EXPECT_NE(convert_back_data.content_size, 0);
  EXPECT_EQ(axis_c_string_is_equal_with_size(
                src_str, (const char *)convert_back_data.data,
                convert_back_data.content_size),
            true);

  axis_buf_deinit(&convert_back_data);
  axis_string_deinit(&result);
}
