//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/thread.h"

#define axis_MUTEX_SIGNATURE 0x0EFAAE3C19611249U

/**
 * @brief Set the owner thread.
 * @param mutex The mutex handle.
 */
axis_UTILS_PRIVATE_API void axis_mutex_set_owner(axis_mutex_t *mutex,
                                               axis_tid_t owner);
