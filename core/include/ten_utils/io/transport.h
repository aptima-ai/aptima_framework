//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/mutex.h"

typedef struct ten_transportbackend_t ten_transportbackend_t;
typedef struct ten_string_t ten_string_t;
typedef struct ten_stream_t ten_stream_t;

typedef enum TEN_TRANSPORT_DROP_TYPE {
  /* Drop oldest data when transport channel is full, only available when
     transport type is shm or raw pointer */
  TEN_TRANSPORT_DROP_OLD,

  /* Drop current data when transport channel is full */
  TEN_TRANSPORT_DROP_NEW,

  /* Drop current data if no reader is waiting */
  TEN_TRANSPORT_DROP_IF_NO_READER,

  /* Drop current data by asking, only available when
     transport type is shm or raw pointer .
     Useful if user wan't to drop by bussiness logic (e.g. never drop I frame)
   */
  TEN_TRANSPORT_DROP_ASK
} TEN_TRANSPORT_DROP_TYPE;

typedef struct ten_transport_t ten_transport_t;

struct ten_transport_t {
  /**
   * uv loop that attached to current thread
   */
  ten_runloop_t *loop;

  void *user_data;

  ten_transportbackend_t *backend;
  ten_atomic_t close;

  ten_mutex_t *lock;
  int drop_when_full;
  TEN_TRANSPORT_DROP_TYPE drop_type;

  /**
   * Callback when a new rx stream is connected
   */
  void (*on_server_connected)(ten_transport_t *transport, ten_stream_t *stream,
                              int status);

  /**
   * Callback when a new rx stream is created
   */
  void (*on_client_accepted)(ten_transport_t *transport, ten_stream_t *stream,
                             int status);

  /**
   * Callback when transport closed
   */
  void (*on_closed)(void *on_closed_data);
  void *on_closed_data;
};

// Public interface
TEN_UTILS_API ten_transport_t *ten_transport_create(ten_runloop_t *loop);

TEN_UTILS_API int ten_transport_close(ten_transport_t *self);

TEN_UTILS_API void ten_transport_set_close_cb(ten_transport_t *self,
                                              void *close_cb,
                                              void *close_cb_data);

TEN_UTILS_API int ten_transport_listen(ten_transport_t *self,
                                       const ten_string_t *my_uri);

TEN_UTILS_API int ten_transport_connect(ten_transport_t *self,
                                        ten_string_t *dest);
