//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "axis_utils/axis_config.h"

axis_UTILS_PRIVATE_API int elf_fetch_bits(const unsigned char **ppin,
                                         const unsigned char *pinend,
                                         uint64_t *pval, unsigned int *pbits);
