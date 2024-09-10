//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_runtime/ten_config.h"

#include <assert.h>

#include "gtest/gtest.h"
#include "include_internal/ten_runtime/msg/msg.h"
#include "ten_runtime/common/status_code.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/string.h"

static inline void check_status_code_is(ten_json_t *json,
                                        TEN_STATUS_CODE status_code) {
  TEN_ASSERT(json, "Should not happen.");

  EXPECT_EQ(
      ten_msg_json_get_integer_field_in_ten(json, "status_code") == status_code,
      true);
}

static inline void check_detail_is_string(ten_json_t *json,
                                          const char *detail) {
  TEN_ASSERT(json, "Should not happen.");
  EXPECT_EQ(ten_c_string_is_equal(
                ten_msg_json_get_string_field_in_ten(json, "detail"), detail),
            true);
}

static inline void check_seq_id_is(ten_json_t *json, const char *expect) {
  TEN_ASSERT(json, "Should not happen.");
  EXPECT_EQ(ten_c_string_is_equal(
                expect, ten_msg_json_get_string_field_in_ten(json, "seq_id")),
            true);
}

static inline void check_result_is(ten_json_t *resp, const char *seq_id,
                                   TEN_STATUS_CODE status_code,
                                   const char *detail) {
  check_seq_id_is(resp, seq_id);
  check_status_code_is(resp, status_code);
  check_detail_is_string(resp, detail);
}

static inline void check_result_json_is(ten_json_t *resp, const char *seq_id,
                                        TEN_STATUS_CODE status_code,
                                        const char *detail_field,
                                        const char *detail_value) {
  check_seq_id_is(resp, seq_id);
  check_status_code_is(resp, status_code);
  ten_json_t *detail = ten_json_from_string(
      ten_msg_json_get_string_field_in_ten(resp, "detail"), NULL);
  EXPECT_EQ(ten_c_string_is_equal(detail_value, ten_json_object_peek_string(
                                                    detail, detail_field)),
            true);
  ten_json_destroy(detail);
}
