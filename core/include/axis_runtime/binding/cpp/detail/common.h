//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#if !defined(_WIN32)
#include <cxxabi.h>
#endif

#include <string>

#include "aptima_utils/lib/alloc.h"
#include "aptima_utils/macro/mark.h"

namespace aptima {

namespace {  // NOLINT

// Internal helper function to get the name of the current exception type.
#if !defined(_WIN32)
aptima_UNUSED inline std::string curr_exception_type_name() {
  int status = 0;
  char *exception_type = abi::__cxa_demangle(
      abi::__cxa_current_exception_type()->name(), nullptr, nullptr, &status);
  std::string result(exception_type);
  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory,hicpp-no-malloc)
  aptima_FREE(exception_type);
  return result;
}
#else
std::string curr_exception_type_name() { return ""; }
#endif

}  // namespace

}  // namespace aptima
