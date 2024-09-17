//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/protocol/protocol.h"
#include "ten_runtime/protocol/context_store.h"
#include "ten_utils/container/hash_handle.h"
#include "ten_utils/container/hash_table.h"
#include "ten_utils/lib/rwlock.h"

#define TEN_PROTOCOL_CONTEXT_STORE_SIGNATURE 0xAD7D3789B3FD89DEU

typedef struct ten_protocol_context_t ten_protocol_context_t;
typedef struct ten_app_t ten_app_t;

typedef void (*ten_protocol_context_store_on_closed_func_t)(
    ten_protocol_context_store_t *self, void *on_closed_data);

/**
 * @brief A protocol context item can store multiple protocol_context(s).
 */
typedef struct ten_protocol_context_store_item_t {
  ten_hashhandle_t hh_in_context_store;

  ten_list_t contexts;  // ten_protocol_context_t*
} ten_protocol_context_store_item_t;

/**
 * @brief The relationship graph is as follows.
 *
 *   protocol_context_store
 *     -> protocol_context_store_item (corresponds to a protocol, or a protocol
 *        + some other values)
 *          -> protocol_context
 *          -> protocol_context
 *          -> protocol_context
 *          -> ...
 *     -> protocol_context_store_item (corresponds to a protocol, or a protocol
 *        + some other values)
 *          -> protocol_context
 *          -> ...
 *     -> ...
 *
 * Ex:
 * ====================
 * Case 1:
 * ====================
 *   protocol_context_store
 *     -> protocol_context_store_item (http_libws)
 *          -> protocol_context
 *          -> protocol_context
 *          -> protocol_context
 *          -> ...
 *     -> protocol_context_store_item (xxx)
 *          -> protocol_context
 *          -> ...
 *     -> ...
 *
 * ====================
 * Case 2:
 * ====================
 *   protocol_context_store
 *     -> protocol_context_store_item (http_libws+"SERVER")
 *          -> protocol_context
 *          -> protocol_context
 *          -> protocol_context
 *          -> ...
 *     -> protocol_context_store_item (http_libws+"CLIENT")
 *          -> protocol_context
 *          -> protocol_context
 *          -> protocol_context
 *          -> ...
 *     -> protocol_context_store_item (xxx)
 *          -> protocol_context
 *          -> ...
 *     -> ...
 *
 * The protocol implementation can freely decide which case it wants to use.
 * - In case 1, if http_libws protocol implementation wants to get "SERVER"-type
 *   protocol context, there must be some 'variable' in those protocol
 *   context(s) to be distinguished.
 * - In case 2, http_libws protocol implementation can use an extra "SERVER" key
 *   to get all "SERVER"-type protocol context(s) at once.
 */
typedef struct ten_protocol_context_store_t {
  ten_signature_t signature;
  ten_sanitizer_thread_check_t thread_check;

  // The owner of 'ten_protocol_context_store_t'.
  //
  // According to the comments above, there are two kinds of protocol context
  // - 'server' protocol context
  // - 'client' protocol context.
  //
  // A protocol context can only be created in the following 2 cases:
  //
  // - ten_protocol_t::listen()
  //   This is the standard api to start a server. If the server needs to run
  //   in its own thread, the thread should be created in this function, and so
  //   does the 'server' protocol context. The ten_protocol_t::listen() function
  //   is called in the TEN app thread.
  //
  // - ten_protocol_t::connect_to()
  //   This is the standard api to create a client and try to connect to the
  //   remote server. And if the client needs to run in its own thread, the
  //   thread should be created in this function, and so does the 'client'
  //   protocol context. The ten_protocol_t::connect_to() function is called in
  //   the TEN engine thread.
  //
  // Because the 'server' and 'client' protocol context maybe same in some
  // cases, which means the server and clients of the implementation protocol
  // are running in the same thread. If the server protocol instance starts
  // first, the client protocol instance wants to get the protocol context
  // created by the server. That's why the owner of
  // 'ten_protocol_context_store_t' can not be 'ten_engine_t' in
  // ten_protocol_t::connect_to().
  ten_app_t *app;

  // When a protocol context is closed, the protocol thread needs to notify the
  // ten_app_t about this, and enable the app thread to remove this closed
  // protocol context from their data structure, so we keep the runloop of
  // ten_app_t here.
  ten_runloop_t *attached_runloop;

  // key: the name of the protocol in its 'manifest.json'.
  // value: ten_protocol_context_store_item_t.
  ten_hashtable_t table;

  ten_rwlock_t *store_lock;

  ten_protocol_context_store_on_closed_func_t on_closed;
  void *on_closed_data;

  bool is_closed;
} ten_protocol_context_store_t;

TEN_RUNTIME_PRIVATE_API ten_protocol_context_store_t *
ten_protocol_context_store_create(ptrdiff_t offset);

TEN_RUNTIME_PRIVATE_API void ten_protocol_context_store_set_on_closed(
    ten_protocol_context_store_t *self,
    ten_protocol_context_store_on_closed_func_t on_closed,
    void *on_closed_data);

TEN_RUNTIME_PRIVATE_API void ten_protocol_context_store_destroy(
    ten_protocol_context_store_t *self);

TEN_RUNTIME_PRIVATE_API void ten_protocol_context_store_close(
    ten_protocol_context_store_t *self);

TEN_RUNTIME_PRIVATE_API bool ten_protocol_context_store_is_closed(
    ten_protocol_context_store_t *self);
