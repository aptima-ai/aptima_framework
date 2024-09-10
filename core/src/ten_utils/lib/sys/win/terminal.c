//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/terminal.h"

#include <Windows.h>
#include <assert.h>
#include <io.h>

size_t ten_terminal_get_width_in_char(void) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  int ret = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  if (ret) {
    return csbi.dwSize.X;
  }

  return 0;
}

int ten_terminal_is_terminal(int fd) { return _isatty(fd); }
