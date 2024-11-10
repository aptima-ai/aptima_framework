//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/path/result_return_policy.h"

#include "include_internal/ten_runtime/common/constant_str.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"

TEN_RESULT_RETURN_POLICY ten_result_return_policy_from_string(
    const char *policy_str) {
  TEN_ASSERT(policy_str, "Invalid argument.");

  if (ten_c_string_is_equal(policy_str, TEN_STR_FIRST_ERROR_OR_FIRST_OK)) {
    return TEN_RESULT_RETURN_POLICY_FIRST_ERROR_OR_FIRST_OK;
  } else if (ten_c_string_is_equal(policy_str,
                                   TEN_STR_FIRST_ERROR_OR_LAST_OK)) {
    return TEN_RESULT_RETURN_POLICY_FIRST_ERROR_OR_LAST_OK;
  } else if (ten_c_string_is_equal(policy_str, TEN_STR_EACH_OK_AND_ERROR)) {
    return TEN_RESULT_RETURN_POLICY_EACH_OK_AND_ERROR;
  } else {
    return TEN_RESULT_RETURN_POLICY_INVALID;
  }
}

const char *ten_result_return_policy_to_string(
    TEN_RESULT_RETURN_POLICY policy) {
  switch (policy) {
    case TEN_RESULT_RETURN_POLICY_FIRST_ERROR_OR_FIRST_OK:
      return TEN_STR_FIRST_ERROR_OR_FIRST_OK;
    case TEN_RESULT_RETURN_POLICY_FIRST_ERROR_OR_LAST_OK:
      return TEN_STR_FIRST_ERROR_OR_LAST_OK;
    case TEN_RESULT_RETURN_POLICY_EACH_OK_AND_ERROR:
      return TEN_STR_EACH_OK_AND_ERROR;
    default:
      return NULL;
  }
}
