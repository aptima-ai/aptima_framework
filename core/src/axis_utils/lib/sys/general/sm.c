//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/sm.h"

#include <string.h>

#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/mutex.h"

struct axis_sm_t {
  axis_mutex_t *lock;
  axis_sm_state_entry_t *entries;
  size_t entry_count;
  axis_sm_auto_trans_t *auto_trans;
  size_t trans_count;
  int current_state;
  int begin_state;
  axis_sm_op default_op;
  axis_list_t history;
  int initted;
};

axis_sm_t *axis_state_machine_create() {
  axis_sm_t *sm = NULL;

  sm = axis_malloc(sizeof(*sm));
  if (!sm) {
    goto error;
  }

  memset(sm, 0, sizeof(*sm));
  sm->lock = axis_mutex_create();
  if (!sm->lock) {
    goto error;
  }

  axis_list_init(&sm->history);

  return sm;

error:
  axis_state_machine_destroy(sm);

  return NULL;
}

void axis_state_machine_destroy(axis_sm_t *sm) {
  if (!sm) {
    return;
  }

  if (sm->entries && sm->entry_count) {
    axis_free(sm->entries);
  }

  if (sm->auto_trans && sm->trans_count) {
    axis_free(sm->auto_trans);
  }

  axis_list_clear(&sm->history);

  if (sm->lock) {
    axis_mutex_destroy(sm->lock);
  }

  axis_free(sm);
}

int axis_state_machine_init(axis_sm_t *sm, int begin_state, axis_sm_op default_op,
                           const axis_sm_state_entry_t *entries,
                           size_t entry_count, const axis_sm_auto_trans_t *trans,
                           size_t trans_count) {
  int index = 0;
  axis_sm_state_entry_t *entry_clone = NULL;
  axis_sm_auto_trans_t *trans_clone = NULL;

  if (!sm || !entries || !entry_count || !sm->lock) {
    goto error;
  }

  for (index = 0; index < entry_count; index++) {
    if (entries[index].current == begin_state) {
      break;
    }
  }

  if (index == entry_count) {
    goto error;
  }

  entry_clone = axis_malloc(entry_count * sizeof(axis_sm_state_entry_t));
  if (!entry_clone) {
    goto error;
  }

  memcpy(entry_clone, entries, entry_count * sizeof(axis_sm_state_entry_t));

  if (trans && trans_count) {
    trans_clone = axis_malloc(trans_count * sizeof(axis_sm_auto_trans_t));
    if (!trans_clone) {
      goto error;
    }

    memcpy(trans_clone, trans, trans_count * sizeof(axis_sm_auto_trans_t));
  }

  axis_mutex_lock(sm->lock);
  if (sm->entries || sm->entry_count || sm->initted) {
    goto leave_and_error;
  }

  sm->entries = entry_clone;
  sm->entry_count = entry_count;
  sm->auto_trans = trans_clone;
  sm->trans_count = trans_count;
  sm->current_state = begin_state;
  sm->begin_state = begin_state;
  sm->default_op = default_op;
  sm->initted = 1;
  axis_mutex_unlock(sm->lock);
  return 0;

leave_and_error:
  axis_mutex_unlock(sm->lock);
error:
  if (entry_clone) {
    axis_free(entry_clone);
  }

  if (trans_clone) {
    axis_free(trans_clone);
  }

  return -1;
}

int axis_state_machine_reset_state(axis_sm_t *sm) {
  if (!sm) {
    goto error;
  }

  axis_mutex_lock(sm->lock);
  if (!sm->entries || !sm->entry_count || !sm->initted) {
    goto leave_and_error;
  }

  sm->current_state = sm->begin_state;
  axis_mutex_unlock(sm->lock);
  return 0;

leave_and_error:
  axis_mutex_unlock(sm->lock);
error:
  return -1;
}

int axis_state_machine_trigger(axis_sm_t *sm, int event, int reason, void *arg) {
  axis_sm_op op = NULL;
  axis_sm_state_entry_t *entry = NULL;
  axis_sm_state_history_t *new_story = NULL;
  int origin_state = -1;
  int next_state = -1;
  int index = 0;

  if (!sm) {
    goto error;
  }

  new_story = axis_malloc(sizeof(*new_story));
  if (!new_story) {
    goto error;
  }

  axis_mutex_lock(sm->lock);
  if (!sm->entries || !sm->entry_count || !sm->initted) {
    goto leave_and_error;
  }

  for (index = 0; index < sm->entry_count; index++) {
    axis_sm_state_entry_t *e = &sm->entries[index];
    if (e->current == sm->current_state && e->event == event &&
        (e->reason == reason || e->reason == axis_REASON_ANY)) {
      entry = e;
      break;
    }
  }

  if (entry) {
    next_state = entry->next;
    op = entry->operation;
  } else {
    next_state = sm->current_state;
    op = sm->default_op;
  }

  origin_state = sm->current_state;
  sm->current_state = next_state;
  axis_mutex_unlock(sm->lock);

  // perform action _without_ lock
  if (op) {
    new_story->event = event;
    new_story->from = origin_state;
    new_story->to = next_state;
    new_story->reason = reason;

    if (axis_list_size(&sm->history) >= axis_SM_MAX_HISTORY) {
      axis_listnode_destroy(axis_list_pop_front(&sm->history));
    }

    axis_list_push_ptr_back(&sm->history, new_story, axis_free);
    op(sm, new_story, arg);
  }

  axis_mutex_lock(sm->lock);

  if (sm->trans_count && sm->auto_trans) {
    for (index = 0; index < sm->trans_count; index++) {
      axis_sm_auto_trans_t *trans = &sm->auto_trans[index];
      int auto_event = trans->auto_trigger;
      int auto_reason = trans->trigger_reason;
      if (trans->from_state != origin_state || trans->to_state != next_state) {
        continue;
      }

      axis_mutex_unlock(sm->lock);
      axis_state_machine_trigger(sm, auto_event, auto_reason, arg);
      axis_mutex_lock(sm->lock);
    }
  }

  axis_mutex_unlock(sm->lock);

  return 0;

leave_and_error:
  axis_mutex_unlock(sm->lock);

error:
  if (new_story) {
    axis_free(new_story);
  }

  return -1;
}

int axis_state_machine_current_state(const axis_sm_t *sm) {
  int state = -1;
  axis_mutex_lock(sm->lock);
  state = sm->current_state;
  axis_mutex_unlock(sm->lock);
  return state;
}