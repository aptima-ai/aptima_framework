//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#if defined(_WIN32)
  #define ten_alignof __alignof
#else
  #include <stdalign.h>
  #define ten_alignof alignof
#endif

#include <stddef.h>

// Utility for aligning addresses.
inline size_t ten_align_forward(size_t addr, size_t align) {
  return (addr + (align - 1)) & ~(align - 1);
}
