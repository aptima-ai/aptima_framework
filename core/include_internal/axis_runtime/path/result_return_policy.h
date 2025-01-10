//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#define axis_DEFAULT_RESULT_RETURN_POLICY \
  axis_RESULT_RETURN_POLICY_EACH_OK_AND_ERROR

typedef enum axis_RESULT_RETURN_POLICY {
  axis_RESULT_RETURN_POLICY_INVALID,

  // If receive a fail result, return it, otherwise, when all OK results are
  // received, return the first received one. Clear the group after returning
  // the result.
  axis_RESULT_RETURN_POLICY_FIRST_ERROR_OR_FIRST_OK,

  // Similar to the above, except return the last received one.
  axis_RESULT_RETURN_POLICY_FIRST_ERROR_OR_LAST_OK,

  // Return each result as it is received, regardless of its status.
  axis_RESULT_RETURN_POLICY_EACH_OK_AND_ERROR,

  // More modes is allowed, and could be added here in case needed.
} axis_RESULT_RETURN_POLICY;

axis_RUNTIME_PRIVATE_API axis_RESULT_RETURN_POLICY
axis_result_return_policy_from_string(const char *policy_str);

axis_RUNTIME_PRIVATE_API const char *axis_result_return_policy_to_string(
    axis_RESULT_RETURN_POLICY policy);
