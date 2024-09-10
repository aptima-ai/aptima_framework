//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <string>

#include "gtest/gtest.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/log/log.h"

TEST(LogTest, FileOutput1) {  // NOLINT
  ten_log_output_t save;
  ten_log_output_init(&save, 0, nullptr, nullptr, nullptr);

  ten_log_save_output_spec(&save);

  ten_log_set_output_to_file("test1.log");

  TEN_LOGE("test %s test", "hello");

  ten_log_close();
  ten_log_restore_output_spec(&save);
}

TEST(LogTest, FileOutput2) {  // NOLINT
  ten_log_t *log = ten_log_create();
  ten_log_set_output_to_file_aux(log, "test2.log");

  TEN_LOGE_AUX(log, "test %s test", "hello");

  ten_log_close_aux(log);
  ten_log_destroy(log);
}
