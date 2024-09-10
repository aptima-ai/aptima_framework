//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#if !defined(_WIN32)
  #include <cxxabi.h>
#endif

#include <string>

#include "ten_utils/lib/alloc.h"
#include "ten_utils/macro/mark.h"

namespace ten {

namespace {  // NOLINT

// Internal helper function to get the name of the current exception type.
#if !defined(_WIN32)
TEN_UNUSED inline std::string curr_exception_type_name() {
  int status = 0;
  char *exception_type = abi::__cxa_demangle(
      abi::__cxa_current_exception_type()->name(), nullptr, nullptr, &status);
  std::string result(exception_type);
  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory,hicpp-no-malloc)
  TEN_FREE(exception_type);
  return result;
}
#else
std::string curr_exception_type_name() { return ""; }
#endif

}  // namespace

}  // namespace ten
