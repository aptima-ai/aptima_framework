//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/terminal.h"

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

size_t axis_terminal_get_width_in_char(void) {
  struct winsize w;
  ioctl(fileno(stdout), TIOCGWINSZ, &w);
  return w.ws_col;
}

int axis_terminal_is_terminal(int fd) { return isatty(fd); }
