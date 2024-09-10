//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_runtime/ten_config.h"

#include <assert.h>

#include <string>

#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

namespace ten_test {

static inline void assert_json_equals_integer(const nlohmann::json &actual,
                                              const int64_t expected) {
  bool is_equals = false;

  if (actual.is_number()) {
    is_equals = (actual.get<int64_t>() == expected);
  }

  EXPECT_EQ(true, is_equals) << "Assertion failed, expected: " << expected
                             << ", actual: " << actual << std::endl;
}

static inline void assert_json_equals(const nlohmann::json &actual,
                                      const std::string &expected) {
  // CAUTION: we can not use 'EXPECT_EQ(str1, str2)' here, there will be too
  // many useless logs if assertion failed.

  bool is_equals = false;

  if (actual.is_string()) {
    is_equals = (actual.get<std::string>() == expected);
  } else {
    // return_json returns an object value.
    is_equals = (actual == nlohmann::json::parse(expected));
  }

  EXPECT_EQ(true, is_equals) << "Assertion failed, expected: " << expected
                             << ", actual: " << actual << std::endl;
}

static inline void check_status_code_is(const nlohmann::json &json,
                                        const TEN_STATUS_CODE status_code) {
  assert_json_equals_integer(json["_ten"]["status_code"], status_code);
}

static inline void check_seq_id_is(const nlohmann::json &json,
                                   const std::string &expect) {
  assert_json_equals(json["_ten"]["seq_id"], expect);
}

static inline void check_detail_is(const nlohmann::json &actual,
                                   const std::string &expected) {
  assert_json_equals(actual["detail"], expected);
}

static inline void check_result_is(const nlohmann::json &resp,
                                   const std::string &seq_id,
                                   const TEN_STATUS_CODE status_code,
                                   const std::string &detail) {
  check_seq_id_is(resp, seq_id);
  check_status_code_is(resp, status_code);
  check_detail_is(resp, detail);
}

static inline void check_result_json_is(const nlohmann::json &resp,
                                        const std::string &seq_id,
                                        const TEN_STATUS_CODE status_code,
                                        const std::string &detail_field,
                                        const std::string &detail_value) {
  check_seq_id_is(resp, seq_id);
  check_status_code_is(resp, status_code);

  nlohmann::json detail =
      nlohmann::json::parse(resp["detail"].get<std::string>());

  assert_json_equals(detail[detail_field], detail_value);
}

}  // namespace ten_test
