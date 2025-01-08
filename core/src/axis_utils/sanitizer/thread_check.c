//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/sanitizer/thread_check.h"

#include <string.h>

#include "include_internal/axis_utils/sanitizer/thread_check.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

bool axis_sanitizer_thread_check_check_integrity(
    axis_sanitizer_thread_check_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_SANITIZER_THREAD_CHECK_SIGNATURE) {
    return false;
  }
  return true;
}

void axis_sanitizer_thread_check_init(axis_sanitizer_thread_check_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_signature_set(&self->signature, axis_SANITIZER_THREAD_CHECK_SIGNATURE);
  self->belonging_thread = NULL;
  self->is_fake = false;
}

void axis_sanitizer_thread_check_init_with_current_thread(
    axis_sanitizer_thread_check_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_sanitizer_thread_check_init(self);

  self->belonging_thread = axis_thread_self();
  if (self->belonging_thread == NULL) {
    // The current thread was not created by axis_thread_create(). We create a
    // fake axis_thread_t with relevant information, but it doesn't actually
    // create a native thread.
    self->belonging_thread = axis_thread_create_fake("fake");
    self->is_fake = true;
  }
}

void axis_sanitizer_thread_check_init_from(axis_sanitizer_thread_check_t *self,
                                          axis_sanitizer_thread_check_t *other) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(other && axis_sanitizer_thread_check_check_integrity(other),
             "Invalid argument.");
  axis_ASSERT(other->belonging_thread, "Should not happen.");

  axis_sanitizer_thread_check_init(self);
  axis_sanitizer_thread_check_inherit_from(self, other);
}

void axis_sanitizer_thread_check_set_belonging_thread(
    axis_sanitizer_thread_check_t *self, axis_thread_t *thread) {
  axis_ASSERT(self && axis_sanitizer_thread_check_check_integrity(self),
             "Should not happen.");

  if (self->belonging_thread && self->is_fake) {
    axis_thread_join_fake(self->belonging_thread);
    self->is_fake = false;
  }

  if (!thread) {
    thread = axis_thread_self();
  }

  self->belonging_thread = thread;
}

void axis_sanitizer_thread_check_set_belonging_thread_to_current_thread(
    axis_sanitizer_thread_check_t *self) {
  axis_ASSERT(self && axis_sanitizer_thread_check_check_integrity(self),
             "Should not happen.");

  if (self->belonging_thread && self->is_fake) {
    axis_thread_join_fake(self->belonging_thread);
    self->is_fake = false;
  }

  self->belonging_thread = axis_thread_self();

  if (self->belonging_thread == NULL) {
    // The current thread was not created by axis_thread_create(). We create a
    // fake axis_thread_t with relevant information, but it doesn't actually
    // create a native thread.
    self->belonging_thread = axis_thread_create_fake("fake");
    self->is_fake = true;
  }
}

axis_thread_t *axis_sanitizer_thread_check_get_belonging_thread(
    axis_sanitizer_thread_check_t *self) {
  axis_ASSERT(self && axis_sanitizer_thread_check_check_integrity(self),
             "Should not happen.");
  return self->belonging_thread;
}

void axis_sanitizer_thread_check_inherit_from(
    axis_sanitizer_thread_check_t *self, axis_sanitizer_thread_check_t *from) {
  axis_ASSERT(from && axis_sanitizer_thread_check_check_integrity(from),
             "Should not happen.");
  axis_ASSERT(from->belonging_thread, "Should not happen.");
  axis_ASSERT(self && axis_sanitizer_thread_check_check_integrity(self),
             "Should not happen.");

  if (self->belonging_thread && self->is_fake) {
    axis_thread_join_fake(self->belonging_thread);
    self->is_fake = false;
  }

  self->belonging_thread = from->belonging_thread;
}

bool axis_sanitizer_thread_check_do_check(axis_sanitizer_thread_check_t *self) {
  axis_ASSERT(self && axis_sanitizer_thread_check_check_integrity(self),
             "Should not happen.");

  if (!self->belonging_thread) {
    // belonging_thread has not been set, which means we don't want to do thread
    // safety checks at this time.
    return true;
  }

  axis_thread_t *current_thread = axis_thread_self();

  if (axis_thread_equal(current_thread, self->belonging_thread)) {
    return true;
  }

  axis_LOGE(
      "Access object across threads, current thread {id(%ld), name(%s)}, "
      "but belonging thread {id(%ld), name(%s)}.",
      (long)axis_thread_get_id(current_thread),
      axis_thread_get_name(current_thread),
      (long)axis_thread_get_id(self->belonging_thread),
      axis_thread_get_name(self->belonging_thread));

  return false;
}

void axis_sanitizer_thread_check_deinit(axis_sanitizer_thread_check_t *self) {
  axis_ASSERT(self && axis_sanitizer_thread_check_check_integrity(self),
             "Should not happen.");

  if (self->is_fake) {
    axis_thread_join_fake(self->belonging_thread);
  }
  self->belonging_thread = NULL;
}
