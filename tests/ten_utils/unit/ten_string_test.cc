//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <string>

#include "gtest/gtest.h"
#include "ten_utils/lib/string.h"

TEST(StringTest, equal) {
  const std::string test_str1 = "testing string";
  const std::string test_str2 = "testing String";

  auto s = ten_string_create_formatted(test_str1.c_str());
  auto r = ten_string_is_equal_c_str(s, test_str1.c_str());
  EXPECT_EQ(r, true);

  r = ten_string_is_equal_c_str(s, test_str2.c_str());
  EXPECT_EQ(r, false);

  r = ten_string_is_equal_c_str_case_insensitive(s, test_str2.c_str());
  EXPECT_EQ(r, true);

  ten_string_destroy(s);
}

TEST(StringTest, concat) {
  const std::string test_str1 = "testing string";
  const std::string test_str2 = "testing String";

  auto s = ten_string_create_formatted(test_str1.c_str());

  ten_string_append_formatted(s, "%.*s", test_str2.length(), test_str2.c_str());
  auto r = ten_string_is_equal_c_str(s, (test_str1 + test_str2).c_str());
  EXPECT_EQ(r, true);

  ten_string_destroy(s);
}
