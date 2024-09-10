//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/thread_once.h"

#include <Windows.h>

typedef void (*init_routine)(void);

static BOOL win32_once(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) {
  init_routine routine = (init_routine)Parameter;

  if (!Parameter) {
    return FALSE;
  }

  routine();

  return TRUE;
}

int ten_thread_once(ten_thread_once_t *once_control, init_routine routine) {
  LPVOID context = NULL;

  return InitOnceExecuteOnce(once_control, (PINIT_ONCE_FN)win32_once,
                             (PVOID)routine, &context)
             ? 0
             : -1;
}
