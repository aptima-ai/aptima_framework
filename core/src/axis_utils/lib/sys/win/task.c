//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/task.h"

#include <Windows.h>

axis_pid_t axis_task_get_id() { return (axis_pid_t)GetCurrentProcessId(); }