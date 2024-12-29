//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/ref.h"

#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

static bool ten_ref_check_integrity(ten_ref_t *self,
                                    bool has_positive_ref_cnt) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) != TEN_REF_SIGNATURE) {
    return false;
  }

  if (!self->on_end_of_life) {
    return false;
  }

  if (!self->supervisee) {
    return false;
  }

  // The reference count should ba always > 0 before ten_ref_deinit().
  size_t minimum_ref_cnt = has_positive_ref_cnt ? 1 : 0;
  if (ten_atomic_load(&self->ref_cnt) < minimum_ref_cnt) {
    return false;
  }

  return true;
}

void ten_ref_deinit(ten_ref_t *self) {
  TEN_ASSERT(self && ten_ref_check_integrity(
                         self, /* The reference count is zero now. */ false),
             "Invalid argument.");

  // Reset the member fields, so further integrity checking would be failed.
  ten_signature_set(&self->signature, 0);
  ten_atomic_store(&self->ref_cnt, 0);
  self->supervisee = NULL;
  self->on_end_of_life = NULL;
}

void ten_ref_destroy(ten_ref_t *self) {
  TEN_ASSERT(self && ten_ref_check_integrity(
                         self, /* The reference count is zero now. */ false),
             "Invalid argument.");

  ten_ref_deinit(self);

  TEN_FREE(self);
}

void ten_ref_init(ten_ref_t *self, void *supervisee,
                  ten_ref_on_end_of_life_func_t on_end_of_life) {
  TEN_ASSERT(supervisee && on_end_of_life,
             "ten_ref_t needs to manage an object.");

  ten_signature_set(&self->signature, TEN_REF_SIGNATURE);
  self->supervisee = supervisee;
  self->on_end_of_life = on_end_of_life;

  // The initial value of a ten_ref_t instance should be 1.
  ten_atomic_store(&self->ref_cnt, 1);
}

ten_ref_t *ten_ref_create(void *supervisee,
                          ten_ref_on_end_of_life_func_t on_end_of_life) {
  TEN_ASSERT(supervisee && on_end_of_life,
             "ten_ref_t needs to manage an object.");

  ten_ref_t *self = (ten_ref_t *)TEN_MALLOC(sizeof(ten_ref_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_ref_init(self, supervisee, on_end_of_life);

  return self;
}

bool ten_ref_inc_ref(ten_ref_t *self) {
  TEN_ASSERT(self && ten_ref_check_integrity(self, true), "Invalid argument.");

  int64_t old_ref_cnt = ten_atomic_inc_if_non_zero(&self->ref_cnt);
  if (old_ref_cnt == 0) {
    TEN_LOGE("Add a reference to an object that is already dead.");
    TEN_ASSERT(0, "Should not happen.");
    return false;
  }

  return true;
}

bool ten_ref_dec_ref(ten_ref_t *self) {
  TEN_ASSERT(self && ten_ref_check_integrity(self, true), "Invalid argument.");

  int64_t old_ref_cnt = ten_atomic_dec_if_non_zero(&self->ref_cnt);
  if (old_ref_cnt == 0) {
    TEN_LOGE("Delete a reference to an object that is already dead.");
    TEN_ASSERT(0, "Should not happen.");
    return false;
  }

  if (old_ref_cnt == 1) {
    // Call the 'end-of-life' callback of the 'supervisee'.
    TEN_ASSERT(
        self->on_end_of_life,
        "The 'on_end_of_life' must be specified to destroy the supervisee.");

    self->on_end_of_life(self, self->supervisee);
  }

  return true;
}

int64_t ten_ref_get_ref(ten_ref_t *self) {
  TEN_ASSERT(self && ten_ref_check_integrity(self, false), "Invalid argument.");

  return ten_atomic_load(&self->ref_cnt);
}
