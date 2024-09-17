//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "ten_utils/ten_config.h"

TEN_UTILS_PRIVATE_API int elf_fetch_bits(const unsigned char **ppin,
                                         const unsigned char *pinend,
                                         uint64_t *pval, unsigned int *pbits);
