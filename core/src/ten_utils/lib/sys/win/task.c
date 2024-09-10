//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/task.h"

#include <Windows.h>

ten_pid_t ten_task_get_id() { return (ten_pid_t)GetCurrentProcessId(); }