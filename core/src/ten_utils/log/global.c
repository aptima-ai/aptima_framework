//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include <assert.h>
#include <stdlib.h>

#include "include_internal/ten_utils/log/level.h"
#include "include_internal/ten_utils/log/log.h"
#include "include_internal/ten_utils/log/output.h"

ten_log_t ten_global_log = {TEN_LOG_SIGNATURE,
                            TEN_LOG_LEVEL_DEBUG,
                            {ten_log_out_stderr_cb, NULL, NULL}};

void ten_log_global_init(void) { ten_log_init(&ten_global_log); }

void ten_log_global_deinit(void) { ten_log_deinit(&ten_global_log); }

void ten_log_global_set_output_level(TEN_LOG_LEVEL level) {
  ten_log_set_output_level(&ten_global_log, level);
}

void ten_log_global_set_output_to_file(const char *log_path) {
  ten_log_set_output_to_file(&ten_global_log, log_path);
}
