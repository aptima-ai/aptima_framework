//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stddef.h>

#if !defined(NDEBUG)
#define aptima_SM_MAX_HISTORY 10
#else
#define aptima_SM_MAX_HISTORY 1
#endif

typedef struct aptima_sm_t aptima_sm_t;

typedef struct aptima_sm_state_history_t {
  int from;
  int event;
  int reason;
  int to;
} aptima_sm_state_history_t;

typedef void (*aptima_sm_op)(aptima_sm_t *sm, const aptima_sm_state_history_t *top,
                          void *arg);

#define aptima_REASON_ANY (-1)

typedef struct aptima_sm_state_entry_t {
  int current;
  int event;
  int reason;
  int next;
  aptima_sm_op operation;
} aptima_sm_state_entry_t;

typedef struct aptima_sm_auto_trans_t {
  int from_state;
  int to_state;
  int auto_trigger;
  int trigger_reason;
} aptima_sm_auto_trans_t;

aptima_UTILS_API aptima_sm_t *aptima_state_machine_create();

aptima_UTILS_API void aptima_state_machine_destroy(aptima_sm_t *sm);

aptima_UTILS_API int aptima_state_machine_init(aptima_sm_t *sm, int begin_state,
                                         aptima_sm_op default_op,
                                         const aptima_sm_state_entry_t *entries,
                                         size_t entry_count,
                                         const aptima_sm_auto_trans_t *trans,
                                         size_t trans_count);

aptima_UTILS_API int aptima_state_machine_reset_state(aptima_sm_t *sm);

aptima_UTILS_API int aptima_state_machine_trigger(aptima_sm_t *sm, int event, int reason,
                                            void *arg);

aptima_UTILS_API int aptima_state_machine_current_state(const aptima_sm_t *sm);
