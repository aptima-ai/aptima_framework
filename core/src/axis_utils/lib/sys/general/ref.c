//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/ref.h"

#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

static bool axis_ref_check_integrity(axis_ref_t *self,
                                    bool has_positive_ref_cnt) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_REF_SIGNATURE) {
    return false;
  }

  if (!self->on_end_of_life) {
    return false;
  }

  if (!self->supervisee) {
    return false;
  }

  // The reference count should ba always > 0 before axis_ref_deinit().
  size_t minimum_ref_cnt = has_positive_ref_cnt ? 1 : 0;
  if (axis_atomic_load(&self->ref_cnt) < minimum_ref_cnt) {
    return false;
  }

  return true;
}

void axis_ref_deinit(axis_ref_t *self) {
  axis_ASSERT(self && axis_ref_check_integrity(
                         self, /* The reference count is zero now. */ false),
             "Invalid argument.");

  // Reset the member fields, so further integrity checking would be failed.
  axis_signature_set(&self->signature, 0);
  axis_atomic_store(&self->ref_cnt, 0);
  self->supervisee = NULL;
  self->on_end_of_life = NULL;
}

void axis_ref_destroy(axis_ref_t *self) {
  axis_ASSERT(self && axis_ref_check_integrity(
                         self, /* The reference count is zero now. */ false),
             "Invalid argument.");

  axis_ref_deinit(self);

  axis_FREE(self);
}

void axis_ref_init(axis_ref_t *self, void *supervisee,
                  axis_ref_on_end_of_life_func_t on_end_of_life) {
  axis_ASSERT(supervisee && on_end_of_life,
             "axis_ref_t needs to manage an object.");

  axis_signature_set(&self->signature, axis_REF_SIGNATURE);
  self->supervisee = supervisee;
  self->on_end_of_life = on_end_of_life;

  // The initial value of a axis_ref_t instance should be 1.
  axis_atomic_store(&self->ref_cnt, 1);
}

axis_ref_t *axis_ref_create(void *supervisee,
                          axis_ref_on_end_of_life_func_t on_end_of_life) {
  axis_ASSERT(supervisee && on_end_of_life,
             "axis_ref_t needs to manage an object.");

  axis_ref_t *self = (axis_ref_t *)axis_MALLOC(sizeof(axis_ref_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_ref_init(self, supervisee, on_end_of_life);

  return self;
}

bool axis_ref_inc_ref(axis_ref_t *self) {
  axis_ASSERT(self && axis_ref_check_integrity(self, true), "Invalid argument.");

  int64_t old_ref_cnt = axis_atomic_inc_if_non_zero(&self->ref_cnt);
  if (old_ref_cnt == 0) {
    axis_LOGE("Add a reference to an object that is already dead.");
    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  return true;
}

bool axis_ref_dec_ref(axis_ref_t *self) {
  axis_ASSERT(self && axis_ref_check_integrity(self, true), "Invalid argument.");

  int64_t old_ref_cnt = axis_atomic_dec_if_non_zero(&self->ref_cnt);
  if (old_ref_cnt == 0) {
    axis_LOGE("Delete a reference to an object that is already dead.");
    axis_ASSERT(0, "Should not happen.");
    return false;
  }

  if (old_ref_cnt == 1) {
    // Call the 'end-of-life' callback of the 'supervisee'.
    axis_ASSERT(
        self->on_end_of_life,
        "The 'on_end_of_life' must be specified to destroy the supervisee.");

    self->on_end_of_life(self, self->supervisee);
  }

  return true;
}

int64_t axis_ref_get_ref(axis_ref_t *self) {
  axis_ASSERT(self && axis_ref_check_integrity(self, false), "Invalid argument.");

  return axis_atomic_load(&self->ref_cnt);
}
