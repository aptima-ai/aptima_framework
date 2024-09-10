//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <cstring>

#include "common/test_utils.h"
#include "gtest/gtest.h"
#include "ten_utils/lib/random.h"

TEST(RandomTest, RandomTest) {
  char buf1[12];
  char buf2[12];
  for (auto i = 0; i < 20; i++) {
    EXPECT_EQ(ten_random_string(buf1, sizeof(buf1)), 0);
    EXPECT_EQ(ten_random_string(buf2, sizeof(buf2)), 0);
    EXPECT_NE(memcmp(buf1, buf2, 12), 0);
    AGO_LOG("random string: %s\n", buf1);
    AGO_LOG("random string: %s\n", buf2);
  }
}

TEST(RandomTest, RandomHexTest) {
  char buf1[13];
  char buf2[12];
  for (auto i = 0; i < 20; i++) {
    EXPECT_EQ(ten_random_hex_string(buf1, sizeof(buf1)), 0);
    EXPECT_EQ(ten_random_hex_string(buf2, sizeof(buf2)), 0);
    EXPECT_NE(memcmp(buf1, buf2, 12), 0);
    AGO_LOG("random hex string: %s\n", buf1);
    AGO_LOG("random hex string: %s\n", buf2);
  }
}