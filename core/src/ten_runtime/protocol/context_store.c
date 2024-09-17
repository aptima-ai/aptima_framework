//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/protocol/context_store.h"

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/close.h"
#include "include_internal/ten_runtime/protocol/context.h"
#include "include_internal/ten_runtime/protocol/context_store.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/protocol/context.h"
#include "ten_utils/container/hash_table.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/container/list_node_ptr.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/ref.h"
#include "ten_utils/lib/rwlock.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/field.h"
#include "ten_utils/sanitizer/thread_check.h"

bool ten_protocol_context_store_check_integrity(
    ten_protocol_context_store_t *self, bool thread_check) {
  TEN_ASSERT(self, "Invalid argument.");

  if (ten_signature_get(&self->signature) !=
      TEN_PROTOCOL_CONTEXT_STORE_SIGNATURE) {
    return false;
  }

  if (thread_check) {
    return ten_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

static void ten_protocol_context_store_remove_context(
    ten_protocol_context_t *protocol_context) {
  TEN_ASSERT(protocol_context, "Invalid argument.");

  ten_ref_dec_ref(&protocol_context->ref);
}

static ten_protocol_context_store_item_t *
ten_protocol_context_store_item_create(
    ten_protocol_context_t *protocol_context) {
  TEN_ASSERT(protocol_context, "Invalid argument.");

  ten_protocol_context_store_item_t *self =
      (ten_protocol_context_store_item_t *)TEN_MALLOC(
          sizeof(ten_protocol_context_store_item_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_list_init(&self->contexts);

  ten_ref_inc_ref(&protocol_context->ref);
  ten_list_push_ptr_back(&self->contexts, protocol_context,
                         (ten_ptr_listnode_destroy_func_t)
                             ten_protocol_context_store_remove_context);

  return self;
}

static void ten_protocol_context_store_item_destroy(
    ten_protocol_context_store_item_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_list_clear(&self->contexts);

  TEN_FREE(self);
}

ten_protocol_context_store_t *ten_protocol_context_store_create(
    ptrdiff_t offset) {
  ten_protocol_context_store_t *self =
      (ten_protocol_context_store_t *)TEN_MALLOC(
          sizeof(ten_protocol_context_store_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_signature_set(&self->signature, TEN_PROTOCOL_CONTEXT_STORE_SIGNATURE);
  ten_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  ten_hashtable_init(&self->table, offset);
  self->store_lock = ten_rwlock_create(TEN_RW_DEFAULT_FAIRNESS);

  self->app = NULL;
  self->attached_runloop = NULL;

  self->on_closed = NULL;
  self->on_closed_data = NULL;

  self->is_closed = false;

  return self;
}

void ten_protocol_context_store_set_on_closed(
    ten_protocol_context_store_t *self,
    ten_protocol_context_store_on_closed_func_t on_closed,
    void *on_closed_data) {
  TEN_ASSERT(self && on_closed, "Invalid argument.");

  // This function should be called in the TEN app thread.
  TEN_ASSERT(ten_protocol_context_store_check_integrity(self, true),
             "Access across threads.");

  self->on_closed = on_closed;
  self->on_closed_data = on_closed_data;
}

void ten_protocol_context_store_destroy(ten_protocol_context_store_t *self) {
  TEN_ASSERT(self && self->is_closed, "Invalid argument.");

  ten_signature_set(&self->signature, 0);

  ten_sanitizer_thread_check_deinit(&self->thread_check);

  ten_hashtable_deinit(&self->table);

  ten_rwlock_destroy(self->store_lock);
  self->store_lock = NULL;

  TEN_FREE(self);
}

void ten_protocol_context_store_attach_to_app(
    ten_protocol_context_store_t *self, ten_app_t *app) {
  TEN_ASSERT(self && app, "Invalid argument.");

  TEN_ASSERT(ten_app_check_integrity(
                 app,
                 // This function should be called in the app thread.
                 true),
             "Invalid argument.");
  TEN_ASSERT(ten_protocol_context_store_check_integrity(self, true),
             "Invalid argument.");

  self->app = app;
  self->attached_runloop = ten_app_get_attached_runloop(app);
  TEN_ASSERT(self->attached_runloop, "Failed to get runloop of the app.");
}

/**
 * @note This function is _not_ thread safe, so all the calling sites should be
 * protected by 'ten_protocol_context_store_t::store_lock'.
 */
static ten_protocol_context_store_item_t *
ten_protocol_context_store_get_by_name(ten_protocol_context_store_t *self,
                                       const char *protocol_name) {
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The protocol context store is belonging to the app thread,
  // but this function might be called in the engine thread (e.g.: 'connect_to'
  // a remote from an extension). So all the calling sites should use
  // 'ten_protocol_context_store_t::store_lock' to ensure the thread safety.
  TEN_ASSERT(self && ten_protocol_context_store_check_integrity(self, false),
             "Invalid argument.");
  TEN_ASSERT(protocol_name, "Invalid argument.");

  ten_hashhandle_t *found =
      ten_hashtable_find_string(&self->table, protocol_name);

  if (found) {
    return CONTAINER_OF_FROM_FIELD(found, ten_protocol_context_store_item_t,
                                   hh_in_context_store);
  }

  return NULL;
}

static bool ten_protocol_context_store_is_closing(
    ten_protocol_context_store_t *self) {
  TEN_ASSERT(self &&
                 // This function can be called in different threads.
                 ten_protocol_context_store_check_integrity(self, false),
             "Invalid argument.");

  // 'app' pointer should be read-only after the TEN app is creating, so it's
  // safe to 'read' it in different threads.
  ten_app_t *app = self->app;
  return ten_app_is_closing(app);
}

bool ten_protocol_context_store_add_context_if_absent(
    ten_protocol_context_store_t *self,
    ten_protocol_context_t *protocol_context) {
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: Both 'ten_protocol_context_store_t' and
  // 'ten_protocol_context_t' are belonging to the app thread, but this function
  // maybe called in the engine thread (e.g.: 'connect_to' a remote from an
  // extension). So we use 'ten_protocol_context_store_t::store_lock' to ensure
  // the thread safety.
  TEN_ASSERT(self && ten_protocol_context_store_check_integrity(self, false),
             "Access across thread");
  TEN_ASSERT(protocol_context &&
                 ten_protocol_context_check_integrity(protocol_context, false),
             "Invalid argument.");

  if (ten_protocol_context_store_is_closing(self)) {
    return false;
  }

  const char *key_in_store =
      ten_string_get_raw_str(&protocol_context->key_in_store);

  {
    // Check if the belonging protocol representation exists in the store.

    ten_rwlock_lock(self->store_lock, 0);

    ten_protocol_context_store_item_t *item =
        ten_protocol_context_store_get_by_name(self, key_in_store);

    if (item) {
      ten_rwlock_unlock(self->store_lock, 0);
      return false;
    }

    item = ten_protocol_context_store_item_create(protocol_context);
    TEN_ASSERT(item, "Invalid argument.");

    ten_hashtable_add_string(&self->table, &item->hh_in_context_store,
                             key_in_store,
                             ten_protocol_context_store_item_destroy);

    ten_rwlock_unlock(self->store_lock, 0);
  }

  return true;
}

/**
 * @brief Find the implementation protocol context, _not_ the TEN protocol
 * context.
 *
 * @note The item in the protocol context store is the TEN protocol context, not
 * the implementation.
 */
static ten_protocol_context_t *ten_protocol_context_store_find_first_context(
    ten_protocol_context_store_t *self, const char *protocol_name) {
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The protocol context store is belonging to the app thread,
  // but this function maybe called in the engine thread (e.g.: 'connect_to' a
  // remote from an extension). So we use
  // 'ten_protocol_context_store_t::store_lock' to ensure the thread safety.
  TEN_ASSERT(self && ten_protocol_context_store_check_integrity(self, false),
             "Invalid argument.");
  TEN_ASSERT(protocol_name, "Invalid argument.");

  if (ten_protocol_context_store_is_closing(self)) {
    return NULL;
  }

  ten_protocol_context_t *context = NULL;

  {
    ten_rwlock_lock(self->store_lock, 1);

    ten_protocol_context_store_item_t *item =
        ten_protocol_context_store_get_by_name(self, protocol_name);

    if (item) {
      if (ten_list_size(&item->contexts)) {
        ten_listnode_t *node = ten_list_front(&item->contexts);
        TEN_ASSERT(node, "Invalid argument.");

        context = ten_ptr_listnode_get(node);
        TEN_ASSERT(context, "Should not happen.");

        // The protocol context might be used by the protocol even if the
        // protocol context is closed and removed out from the context store. So
        // we have to increase the reference count to prevent from accessing the
        // illegal memory.
        ten_ref_inc_ref(&context->ref);
      }
    }

    ten_rwlock_unlock(self->store_lock, 1);
  }

  return context;
}

/**
 * @brief Find the implementation protocol context, _not_ the TEN protocol
 * context.
 *
 * @note The item in the protocol context store is the TEN protocol context, not
 * the implementation.
 */
ten_protocol_context_t *ten_protocol_context_store_find_first_context_with_role(
    ten_protocol_context_store_t *self, const char *protocol_name,
    TEN_PROTOCOL_ROLE role) {
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The protocol context store is belonging to the app thread,
  // but this function maybe called in the engine thread (e.g.: 'connect_to' a
  // remote from an extension). So we will use
  // 'ten_protocol_context_store_t::store_lock' to ensure the thread safety in
  // 'ten_protocol_context_store_find_first_impl_context()'.
  TEN_ASSERT(self && ten_protocol_context_store_check_integrity(self, false),
             "Invalid argument.");
  TEN_ASSERT(protocol_name && role != TEN_PROTOCOL_ROLE_INVALID,
             "Invalid argument.");

  if (ten_protocol_context_store_is_closing(self)) {
    return NULL;
  }

  ten_string_t key_in_store;
  ten_string_init_formatted(&key_in_store, "%s::%d", protocol_name, role);

  ten_protocol_context_t *context =
      ten_protocol_context_store_find_first_context(
          self, ten_string_get_raw_str(&key_in_store));

  ten_string_deinit(&key_in_store);

  return context;
}

static void ten_protocol_context_store_do_close(
    ten_protocol_context_store_t *self) {
  TEN_ASSERT(self && ten_protocol_context_store_check_integrity(self, true),
             "Access across threads.");

  self->is_closed = true;

  if (self->on_closed) {
    self->on_closed(self, self->on_closed_data);
  }
}

static bool ten_protocol_context_store_could_be_close(
    ten_protocol_context_store_t *self) {
  TEN_ASSERT(self && ten_protocol_context_store_check_integrity(self, true),
             "Invalid argument.");

  if (ten_hashtable_items_cnt(&self->table) == 0) {
    return true;
  }
  return false;
}

static void ten_protocol_context_store_on_close(
    ten_protocol_context_store_t *self) {
  TEN_ASSERT(self && ten_protocol_context_store_check_integrity(self, true),
             "Access across threads.");

  if (ten_protocol_context_store_could_be_close(self)) {
    ten_protocol_context_store_do_close(self);
  }
}

static void ten_protocol_context_store_on_context_closed(
    ten_protocol_context_t *context, void *on_closed_data) {
  ten_protocol_context_store_t *self =
      (ten_protocol_context_store_t *)on_closed_data;
  TEN_ASSERT(context && ten_protocol_context_check_integrity(context, true),
             "Invalid argument.");
  TEN_ASSERT(self && ten_protocol_context_store_check_integrity(self, true),
             "Invalid argument.");

  ten_hashhandle_t *found = ten_hashtable_find_string(
      &self->table, ten_string_get_raw_str(&context->key_in_store));
  TEN_ASSERT(found, "Should not happen.");

  ten_protocol_context_store_item_t *item = CONTAINER_OF_FROM_FIELD(
      found, ten_protocol_context_store_item_t, hh_in_context_store);
  TEN_ASSERT(item, "Invalid argument.");

  // Remove the closed protocol context from the belonging protocol context
  // store.
  ten_list_foreach (&item->contexts, iter) {
    ten_protocol_context_t *saved_context = ten_ptr_listnode_get(iter.node);
    if (saved_context == context) {
      ten_list_remove_node(&item->contexts, iter.node);
      break;
    }
  }

  if (ten_list_is_empty(&item->contexts)) {
    ten_hashtable_del(&self->table, found);

    ten_protocol_context_store_on_close(self);
  }
}

// Close all the protocol contexts maintained by this protocol context store.
void ten_protocol_context_store_close(ten_protocol_context_store_t *self) {
  TEN_ASSERT(self && ten_protocol_context_store_check_integrity(self, true),
             "Access across threads.");
  TEN_ASSERT(ten_protocol_context_store_is_closing(self),
             "Only close context when the app is closing");

  ten_rwlock_lock(self->store_lock, 0);

  // If the protocol context store could be closed directly, the following loop
  // will skip.
  ten_protocol_context_store_on_close(self);

  // Loop for each 'item'.
  ten_hashtable_foreach(&self->table, iter) {
    ten_hashhandle_t *hh = iter.node;
    ten_protocol_context_store_item_t *item =
        CONTAINER_OF_FROM_OFFSET(hh, self->table.hh_offset);
    TEN_ASSERT(item, "Invalid argument.");

    // Close all the protocol_context maintained by this 'item'.
    ten_list_foreach (&item->contexts, iter2) {
      ten_protocol_context_t *saved_context = ten_ptr_listnode_get(iter2.node);
      TEN_ASSERT(saved_context, "Invalid argument.");

      ten_protocol_context_set_on_closed(
          saved_context, ten_protocol_context_store_on_context_closed, self);

      ten_protocol_context_close(saved_context);
    }
  }

  ten_rwlock_unlock(self->store_lock, 0);
}

ten_runloop_t *ten_protocol_context_store_get_attached_runloop(
    ten_protocol_context_store_t *self) {
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function will be called in both the TEN app and engine
  // thread. The 'ten_protocol_context_store_t' is created when starting the TEN
  // app, its attached runloop is the runloop of the TEN app. And
  // 'ten_protocol_context_store_t::attached_runloop' is immutable after the
  // 'ten_protocol_context_store_t' is created. So it should be thread safe to
  // read the 'attached_runloop' field.
  TEN_ASSERT(self && ten_protocol_context_store_check_integrity(self, false),
             "Invalid argument.");

  return self->attached_runloop;
}

bool ten_protocol_context_store_is_closed(ten_protocol_context_store_t *self) {
  TEN_ASSERT(self && ten_protocol_context_store_check_integrity(self, true),
             "Access across threads.");

  return self->is_closed;
}
