//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <thread>

#include "gtest/gtest.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/error.h"

static void cpp_thread() {
  axis_error_t *err = axis_error_create();

  // Not set before, errno is axis_ERRNO_OK.
  EXPECT_EQ(axis_error_errno(err), axis_ERRNO_OK);

  axis_error_set(err, 1, "Error msg in cpp_thread.");
  EXPECT_EQ(axis_error_errno(err), 1);
  EXPECT_STREQ(axis_error_errmsg(err), "Error msg in cpp_thread.");

  axis_error_destroy(err);
}

TEST(TenErrorTest, cpp_thread) {
  axis_error_t *outter_error = axis_error_create();
  axis_error_set(outter_error, axis_ERRNO_INVALID_GRAPH,
                "Incorrect graph definition");

  std::thread t1(cpp_thread);

  EXPECT_STREQ(axis_error_errmsg(outter_error), "Incorrect graph definition");

  t1.join();
  axis_error_destroy(outter_error);
}
