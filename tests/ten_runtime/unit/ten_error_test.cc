//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <thread>

#include "gtest/gtest.h"
#include "ten_runtime/common/errno.h"
#include "ten_utils/lib/error.h"

static void cpp_thread() {
  ten_error_t *err = ten_error_create();

  // Not set before, errno is TEN_ERRNO_OK.
  EXPECT_EQ(ten_error_errno(err), TEN_ERRNO_OK);

  ten_error_set(err, 1, "Error msg in cpp_thread.");
  EXPECT_EQ(ten_error_errno(err), 1);
  EXPECT_STREQ(ten_error_errmsg(err), "Error msg in cpp_thread.");

  ten_error_destroy(err);
}

TEST(TenErrorTest, cpp_thread) {
  ten_error_t *outter_error = ten_error_create();
  ten_error_set(outter_error, TEN_ERRNO_INVALID_GRAPH,
                "Incorrect graph definition");

  std::thread t1(cpp_thread);

  EXPECT_STREQ(ten_error_errmsg(outter_error), "Incorrect graph definition");

  t1.join();
  ten_error_destroy(outter_error);
}
