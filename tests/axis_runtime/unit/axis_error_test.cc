//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <thread>

#include "gtest/gtest.h"
#include "aptima_runtime/common/errno.h"
#include "aptima_utils/lib/error.h"

static void cpp_thread() {
  aptima_error_t *err = aptima_error_create();

  // Not set before, errno is aptima_ERRNO_OK.
  EXPECT_EQ(aptima_error_errno(err), aptima_ERRNO_OK);

  aptima_error_set(err, 1, "Error msg in cpp_thread.");
  EXPECT_EQ(aptima_error_errno(err), 1);
  EXPECT_STREQ(aptima_error_errmsg(err), "Error msg in cpp_thread.");

  aptima_error_destroy(err);
}

TEST(TenErrorTest, cpp_thread) {
  aptima_error_t *outter_error = aptima_error_create();
  aptima_error_set(outter_error, aptima_ERRNO_INVALID_GRAPH,
                "Incorrect graph definition");

  std::thread t1(cpp_thread);

  EXPECT_STREQ(aptima_error_errmsg(outter_error), "Incorrect graph definition");

  t1.join();
  aptima_error_destroy(outter_error);
}
