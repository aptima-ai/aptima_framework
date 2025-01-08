//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stddef.h>

#if !defined(NDEBUG)
#define axis_SM_MAX_HISTORY 10
#else
#define axis_SM_MAX_HISTORY 1
#endif

typedef struct axis_sm_t axis_sm_t;

typedef struct axis_sm_state_history_t {
  int from;
  int event;
  int reason;
  int to;
} axis_sm_state_history_t;

typedef void (*axis_sm_op)(axis_sm_t *sm, const axis_sm_state_history_t *top,
                          void *arg);

#define axis_REASON_ANY (-1)

typedef struct axis_sm_state_entry_t {
  int current;
  int event;
  int reason;
  int next;
  axis_sm_op operation;
} axis_sm_state_entry_t;

typedef struct axis_sm_auto_trans_t {
  int from_state;
  int to_state;
  int auto_trigger;
  int trigger_reason;
} axis_sm_auto_trans_t;

axis_UTILS_API axis_sm_t *axis_state_machine_create();

axis_UTILS_API void axis_state_machine_destroy(axis_sm_t *sm);

axis_UTILS_API int axis_state_machine_init(axis_sm_t *sm, int begin_state,
                                         axis_sm_op default_op,
                                         const axis_sm_state_entry_t *entries,
                                         size_t entry_count,
                                         const axis_sm_auto_trans_t *trans,
                                         size_t trans_count);

axis_UTILS_API int axis_state_machine_reset_state(axis_sm_t *sm);

axis_UTILS_API int axis_state_machine_trigger(axis_sm_t *sm, int event, int reason,
                                            void *arg);

axis_UTILS_API int axis_state_machine_current_state(const axis_sm_t *sm);
