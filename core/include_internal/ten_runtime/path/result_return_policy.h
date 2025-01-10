//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#define TEN_DEFAULT_RESULT_RETURN_POLICY \
  TEN_RESULT_RETURN_POLICY_EACH_OK_AND_ERROR

typedef enum TEN_RESULT_RETURN_POLICY {
  TEN_RESULT_RETURN_POLICY_INVALID,

  // If receive a fail result, return it, otherwise, when all OK results are
  // received, return the first received one. Clear the group after returning
  // the result.
  TEN_RESULT_RETURN_POLICY_FIRST_ERROR_OR_FIRST_OK,

  // Similar to the above, except return the last received one.
  TEN_RESULT_RETURN_POLICY_FIRST_ERROR_OR_LAST_OK,

  // Return each result as it is received, regardless of its status.
  TEN_RESULT_RETURN_POLICY_EACH_OK_AND_ERROR,

  // More modes is allowed, and could be added here in case needed.
} TEN_RESULT_RETURN_POLICY;

TEN_RUNTIME_PRIVATE_API TEN_RESULT_RETURN_POLICY
ten_result_return_policy_from_string(const char *policy_str);

TEN_RUNTIME_PRIVATE_API const char *ten_result_return_policy_to_string(
    TEN_RESULT_RETURN_POLICY policy);
