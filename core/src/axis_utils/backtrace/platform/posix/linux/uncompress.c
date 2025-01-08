//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
// This file is modified from
// https://github.com/ianlancetaylor/libbacktrace [BSD license]
//
#include "include_internal/axis_utils/backtrace/platform/posix/linux/uncompress.h"

/**
 * @brief A function useful for setting a breakpoint for an inflation failure
 * when this code is compiled with -g.
 */
void elf_uncompress_failed(void) {}
