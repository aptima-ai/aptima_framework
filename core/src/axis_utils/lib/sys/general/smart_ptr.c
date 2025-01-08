//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/smart_ptr.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>

#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/macro/check.h"

#define axis_SMART_PTR_SIGNATURE 0x7BB9769E3A5CBA5FU

#define axis_SMART_PTR_COUNTER_RED_ZONE 10000

static bool axis_smart_ptr_check_integrity(axis_smart_ptr_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->ctrl_blk, "The control block should always be valid.");

  if (axis_signature_get(&self->signature) != axis_SMART_PTR_SIGNATURE) {
    return false;
  }

  if (axis_atomic_load(&self->ctrl_blk->shared_cnt) >
      (INT64_MAX - axis_SMART_PTR_COUNTER_RED_ZONE)) {
    // A hint to indicate that we need to consider the wrap back problem.
    return false;
  }

  if (axis_atomic_load(&self->ctrl_blk->weak_cnt) >
      (INT64_MAX - axis_SMART_PTR_COUNTER_RED_ZONE)) {
    // A hint to indicate that we need to consider the wrap back problem.
    return false;
  }

  return true;
}

static bool axis_shared_ptr_check_integrity(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_smart_ptr_check_integrity(self), "Invalid argument.");
  axis_ASSERT(self->type == axis_SMART_PTR_SHARED, "Invalid argument.");
  axis_ASSERT(self->ctrl_blk->shared_cnt, "The shared_ref_cnt should not be 0.");

  return true;
}

static bool axis_weak_ptr_check_integrity(axis_shared_ptr_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_smart_ptr_check_integrity(self), "Invalid argument.");
  axis_ASSERT(self->type == axis_SMART_PTR_WEAK, "Invalid argument.");
  axis_ASSERT(self->ctrl_blk->weak_cnt, "The weak_ref_cnt should not be 0.");

  return true;
}

static void axis_smart_ptr_ctrl_blk_init(axis_smart_ptr_ctrl_blk_t *self,
                                        void *data,
                                        void (*destroy)(void *ptr)) {
  axis_ASSERT(self, "Invalid argument.");

  axis_atomic_store(&self->shared_cnt, 1);

  // A 'counter' structure will be created due to a shared_ptr, and the
  // existence of a shared_ptr will contribute 1 weak_ref_cnt. 'weak_ref_cnt'
  // would be used to determine if the control block ('counter') could be
  // deleted or not.
  axis_atomic_store(&self->weak_cnt, 1);

  self->data = data;
  self->destroy = destroy;
}

static axis_shared_ptr_t *axis_smart_ptr_create_without_ctrl_blk(
    axis_SMART_PTR_TYPE type) {
  axis_ASSERT((type == axis_SMART_PTR_SHARED || type == axis_SMART_PTR_WEAK),
             "Invalid argument.");

  axis_shared_ptr_t *self =
      (axis_shared_ptr_t *)axis_MALLOC(sizeof(axis_shared_ptr_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, axis_SMART_PTR_SIGNATURE);

  self->type = type;

  return self;
}

void *axis_smart_ptr_get_data(axis_smart_ptr_t *self) {
  axis_ASSERT(self && axis_smart_ptr_check_integrity(self), "Invalid argument.");

  void *result = NULL;

  switch (self->type) {
    case axis_SMART_PTR_SHARED:
      result = axis_shared_ptr_get_data(self);
      break;

    case axis_SMART_PTR_WEAK: {
      axis_shared_ptr_t *shared_one = axis_weak_ptr_lock(self);
      axis_ASSERT(shared_one, "Should not happen.");
      if (shared_one) {
        result = axis_shared_ptr_get_data(shared_one);
        axis_shared_ptr_destroy(shared_one);
      }
      break;
    }

    default:
      axis_ASSERT(0, "Invalid argument.");
      break;
  }

  return result;
}

axis_smart_ptr_t *axis_smart_ptr_clone(axis_smart_ptr_t *other) {
  axis_ASSERT(other, "Invalid argument.");
  axis_ASSERT(axis_smart_ptr_check_integrity(other), "Invalid argument.");

  switch (other->type) {
    case axis_SMART_PTR_SHARED:
      return axis_shared_ptr_clone(other);

    case axis_SMART_PTR_WEAK:
      return axis_weak_ptr_clone(other);

    default:
      axis_ASSERT(0, "Invalid argument.");
      return NULL;
  }
}

void axis_smart_ptr_destroy(axis_smart_ptr_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_smart_ptr_check_integrity(self), "Invalid argument.");

  switch (self->type) {
    case axis_SMART_PTR_SHARED: {
      int64_t shared_cnt = axis_atomic_sub_fetch(&self->ctrl_blk->shared_cnt, 1);
      axis_ASSERT(shared_cnt >= 0, "Should not happen.");

      if (shared_cnt == 0) {
        if (self->ctrl_blk->destroy) {
          self->ctrl_blk->destroy(self->ctrl_blk->data);
          self->ctrl_blk->data = NULL;
        }

        int64_t weak_cnt = axis_atomic_sub_fetch(&self->ctrl_blk->weak_cnt, 1);
        axis_ASSERT(weak_cnt >= 0, "Should not happen.");

        if (weak_cnt == 0) {
          axis_FREE(self->ctrl_blk);
          self->ctrl_blk = NULL;
        }
      }
      break;
    }
    case axis_SMART_PTR_WEAK: {
      int64_t weak_cnt = axis_atomic_sub_fetch(&self->ctrl_blk->weak_cnt, 1);
      axis_ASSERT(weak_cnt >= 0, "Should not happen.");

      if (weak_cnt == 0) {
        axis_FREE(self->ctrl_blk);
        self->ctrl_blk = NULL;
      }
      break;
    }
    default:
      axis_ASSERT(0 && "Should not happen.", "Invalid argument.");
      break;
  }

  axis_FREE(self);
}

bool axis_smart_ptr_check_type(axis_smart_ptr_t *self,
                              axis_smart_ptr_type_checker type_checker) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_smart_ptr_check_integrity(self), "Invalid argument.");

  switch (self->type) {
    case axis_SMART_PTR_SHARED:
      return type_checker(axis_shared_ptr_get_data(self));

    case axis_SMART_PTR_WEAK: {
      axis_shared_ptr_t *shared_one = axis_weak_ptr_lock(self);
      if (shared_one) {
        bool rc = type_checker(axis_shared_ptr_get_data(shared_one));
        axis_shared_ptr_destroy(shared_one);
        return rc;
      } else {
        return false;
      }
    }

    default:
      axis_ASSERT(0, "Invalid argument.");
      return false;
  }
}

axis_shared_ptr_t *axis_shared_ptr_create_(void *ptr,
                                         void (*destroy)(void *ptr)) {
  axis_ASSERT(ptr, "Invalid argument.");

  axis_shared_ptr_t *self =
      axis_smart_ptr_create_without_ctrl_blk(axis_SMART_PTR_SHARED);
  axis_ASSERT(self, "Invalid argument.");

  axis_smart_ptr_ctrl_blk_t *ctrl_blk =
      (axis_smart_ptr_ctrl_blk_t *)axis_MALLOC(sizeof(axis_smart_ptr_ctrl_blk_t));
  axis_ASSERT(ctrl_blk, "Failed to allocate memory.");

  axis_smart_ptr_ctrl_blk_init(ctrl_blk, ptr, destroy);
  self->ctrl_blk = ctrl_blk;

  return self;
}

/**
 * @brief Clone a shared_ptr.
 *
 * @note This function expects @a other is valid during the execution of this
 * function.
 */
axis_shared_ptr_t *axis_shared_ptr_clone(axis_shared_ptr_t *other) {
  axis_ASSERT(other, "Invalid argument.");
  axis_ASSERT(axis_shared_ptr_check_integrity(other), "Invalid argument.");

  axis_shared_ptr_t *self =
      axis_smart_ptr_create_without_ctrl_blk(axis_SMART_PTR_SHARED);
  axis_ASSERT(self, "Invalid argument.");

  self->ctrl_blk = other->ctrl_blk;
  axis_atomic_add_fetch(&self->ctrl_blk->shared_cnt, 1);

  return self;
}

void axis_shared_ptr_destroy(axis_shared_ptr_t *self) {
  axis_smart_ptr_destroy(self);
}

void *axis_shared_ptr_get_data(axis_shared_ptr_t *self) {
  axis_ASSERT(self && axis_shared_ptr_check_integrity(self), "Invalid argument.");

  axis_ASSERT(self->ctrl_blk->data != NULL,
             "shared_ptr holds nothing: shared_ref_cnt(%" PRIx64
             "), weak_ref_cnt(%" PRIx64 ")",
             axis_atomic_load(&self->ctrl_blk->shared_cnt),
             axis_atomic_load(&self->ctrl_blk->weak_cnt));

  return self->ctrl_blk->data;
}

// Weak pointer

axis_weak_ptr_t *axis_weak_ptr_create(axis_shared_ptr_t *shared_ptr) {
  axis_ASSERT(shared_ptr && axis_shared_ptr_check_integrity(shared_ptr),
             "Invalid argument.");

  axis_shared_ptr_t *self =
      axis_smart_ptr_create_without_ctrl_blk(axis_SMART_PTR_WEAK);
  axis_ASSERT(self, "Invalid argument.");

  self->ctrl_blk = shared_ptr->ctrl_blk;
  axis_atomic_add_fetch(&self->ctrl_blk->weak_cnt, 1);

  return self;
}

axis_weak_ptr_t *axis_weak_ptr_clone(axis_weak_ptr_t *other) {
  axis_ASSERT(other, "Invalid argument.");
  axis_ASSERT(axis_weak_ptr_check_integrity(other), "Invalid argument.");

  axis_weak_ptr_t *self =
      axis_smart_ptr_create_without_ctrl_blk(axis_SMART_PTR_WEAK);
  axis_ASSERT(self, "Invalid argument.");

  self->ctrl_blk = other->ctrl_blk;
  axis_atomic_add_fetch(&self->ctrl_blk->weak_cnt, 1);

  return self;
}

void axis_weak_ptr_destroy(axis_weak_ptr_t *self) { axis_smart_ptr_destroy(self); }

axis_shared_ptr_t *axis_weak_ptr_lock(axis_weak_ptr_t *self) {
  assert(self && axis_weak_ptr_check_integrity(self));

  int64_t old_shared_ref_cnt =
      axis_atomic_inc_if_non_zero(&self->ctrl_blk->shared_cnt);
  if (old_shared_ref_cnt == 0) {
    // Failed to lock. The smart_ptr is destroyed.
    return NULL;
  } else if (old_shared_ref_cnt > 0) {
    // Create a new sharedptr.
    axis_shared_ptr_t *shared_ptr =
        axis_smart_ptr_create_without_ctrl_blk(axis_SMART_PTR_SHARED);

    // The 'shared_cnt' has already been incremented, therefore, we don't need
    // to increment it anymore.
    shared_ptr->ctrl_blk = self->ctrl_blk;
    return shared_ptr;
  } else {
    axis_ASSERT(0 && "Should not happen.", "Invalid argument.");
    return NULL;
  }
}
