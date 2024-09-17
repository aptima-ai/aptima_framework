//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/protocol/context.h"

#include "include_internal/ten_runtime/protocol/context.h"
#include "include_internal/ten_runtime/protocol/context_store.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/ref.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/macro/memory.h"
#include "ten_utils/sanitizer/thread_check.h"

bool ten_protocol_context_check_integrity(ten_protocol_context_t *self,
                                          bool thread_check) {
  TEN_ASSERT(self, "Invalid argument.");

  if (ten_signature_get(&self->signature) != TEN_PROTOCOL_CONTEXT_SIGNATURE) {
    return false;
  }

  if (!self->impl_protocol_context) {
    return false;
  }

  if (!self->context_store) {
    return false;
  }

  if (thread_check) {
    return ten_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

static void ten_protocol_context_end_of_life(TEN_UNUSED ten_ref_t *ref,
                                             void *supervisee) {
  ten_protocol_context_t *self = (ten_protocol_context_t *)supervisee;

  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The belonging thread might already be stopped, so we do not
  // check the thread integrity.
  TEN_ASSERT(self && ten_protocol_context_check_integrity(self, false),
             "Invalid argument.");

  self->destroy_impl(self->impl_protocol_context);
  self->impl_protocol_context = NULL;

  self->context_store = NULL;
  ten_signature_set(&self->signature, 0);
  ten_string_deinit(&self->key_in_store);
  ten_sanitizer_thread_check_deinit(&self->thread_check);

  ten_ref_deinit(&self->ref);

  TEN_FREE(self);
}

static void ten_protocol_context_init_internal(
    ten_protocol_context_t *self, ten_protocol_context_store_t *context_store,
    ten_protocol_context_close_impl_func_t close_impl,
    ten_protocol_context_destroy_impl_func_t destroy_impl,
    void *impl_protocol_context) {
  TEN_ASSERT(self && context_store && close_impl && destroy_impl,
             "Invalid argument.");

  ten_signature_set(&self->signature, TEN_PROTOCOL_CONTEXT_SIGNATURE);

  // The owner of 'ten_protocol_context_t' is 'ten_protocol_context_store_t',
  // the belonging thread of 'ten_protocol_context_t' should inherit from
  // 'ten_protocol_context_store_t'.
  ten_sanitizer_thread_check_init_from(&self->thread_check,
                                       &context_store->thread_check);

  self->context_store = context_store;

  self->on_closed = NULL;
  self->on_closed_data = NULL;

  ten_atomic_store(&self->is_closing, 0);

  self->impl_is_closed = false;
  self->close_impl = close_impl;
  self->destroy_impl = destroy_impl;
  self->impl_protocol_context = impl_protocol_context;

  ten_ref_init(&self->ref, self, ten_protocol_context_end_of_life);
}

ten_protocol_context_t *ten_protocol_context_create(
    ten_protocol_context_store_t *context_store, const char *protocol_name,
    ten_protocol_context_close_impl_func_t close_impl,
    ten_protocol_context_destroy_impl_func_t destroy_impl,
    void *impl_protocol_context) {
  TEN_ASSERT(
      protocol_name && close_impl && destroy_impl && impl_protocol_context,
      "Invalid argument.");

  ten_protocol_context_t *self = TEN_MALLOC(sizeof(ten_protocol_context_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_string_init_formatted(&self->key_in_store, "%s", protocol_name);

  ten_protocol_context_init_internal(self, context_store, close_impl,
                                     destroy_impl, impl_protocol_context);

  return self;
}

ten_protocol_context_t *ten_protocol_context_create_with_role(
    ten_protocol_context_store_t *context_store, const char *protocol_name,
    TEN_PROTOCOL_ROLE role, ten_protocol_context_close_impl_func_t close_impl,
    ten_protocol_context_destroy_impl_func_t destroy_impl,
    void *impl_protocol_context) {
  TEN_ASSERT(protocol_name && role != TEN_PROTOCOL_ROLE_INVALID && close_impl &&
                 destroy_impl && impl_protocol_context,
             "Invalid argument.");

  ten_protocol_context_t *self = TEN_MALLOC(sizeof(ten_protocol_context_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_string_init_formatted(&self->key_in_store, "%s::%d", protocol_name, role);

  ten_protocol_context_init_internal(self, context_store, close_impl,
                                     destroy_impl, impl_protocol_context);

  return self;
}

void ten_protocol_context_set_on_closed(
    ten_protocol_context_t *self,
    ten_protocol_context_on_closed_func_t on_closed, void *on_closed_data) {
  TEN_ASSERT(
      self && on_closed && ten_protocol_context_check_integrity(self, true),
      "Invalid argument.");

  self->on_closed = on_closed;
  self->on_closed_data = on_closed_data;
}

void ten_protocol_context_close(ten_protocol_context_t *self) {
  TEN_ASSERT(self && ten_protocol_context_check_integrity(self, true),
             "Invalid argument.");

  if (!ten_atomic_bool_compare_swap(&self->is_closing, 0, 1)) {
    return;
  }

  self->close_impl(self->impl_protocol_context);
}

// This function is intended to be called in different threads.
bool ten_protocol_context_is_closing(ten_protocol_context_t *self) {
  TEN_ASSERT(self && ten_protocol_context_check_integrity(self, false),
             "Invalid argument.");

  return ten_atomic_load(&self->is_closing) == 1;
}

static bool ten_protocol_context_could_be_close(ten_protocol_context_t *self) {
  TEN_ASSERT(self && ten_protocol_context_check_integrity(self, true),
             "Invalid argument.");

  return self->impl_is_closed;
}

static void ten_protocol_context_do_close(ten_protocol_context_t *self) {
  TEN_ASSERT(self && ten_protocol_context_check_integrity(self, true),
             "Invalid argument.");

  if (self->on_closed) {
    self->on_closed(self, self->on_closed_data);
  }
}

static void ten_protocol_context_on_close(ten_protocol_context_t *self) {
  TEN_ASSERT(self && ten_protocol_context_check_integrity(self, true),
             "Invalid argument.");

  if (!ten_protocol_context_could_be_close(self)) {
    TEN_LOGD("Could not close alive base protocol context.");
    return;
  }
  TEN_LOGD("Close base protocol context.");

  ten_protocol_context_do_close(self);
}

static void ten_protocol_context_on_implemented_closed_task(
    void *self_, TEN_UNUSED void *arg) {
  ten_protocol_context_t *self = (ten_protocol_context_t *)self_;
  TEN_ASSERT(self && ten_protocol_context_check_integrity(self, true),
             "Invalid argument.");

  TEN_ASSERT(!self->impl_is_closed, "Should not happen.");
  self->impl_is_closed = true;

  if (ten_protocol_context_is_closing(self)) {
    ten_protocol_context_on_close(self);
  }

  ten_ref_dec_ref(&self->ref);
}

/**
 * @brief Call this function once the implementation protocol context is closed.
 * It can be called from any thread.
 */
void ten_protocol_context_on_implemented_closed_async(
    ten_protocol_context_t *self) {
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is called in the 'external protocol thread'
  // when the implementation protocol context has been closed. So we create an
  // async task to ensure the thread safety.
  TEN_ASSERT(self && ten_protocol_context_check_integrity(self, false) &&
                 self->context_store,
             "Invalid argument.");

  ten_runloop_t *runloop =
      ten_protocol_context_store_get_attached_runloop(self->context_store);
  TEN_ASSERT(runloop, "Should not happen.");

  ten_ref_inc_ref(&self->ref);
  ten_runloop_post_task_tail(
      runloop, ten_protocol_context_on_implemented_closed_task, self, NULL);
}
