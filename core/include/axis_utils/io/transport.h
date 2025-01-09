//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include "aptima_utils/io/runloop.h"
#include "aptima_utils/lib/atomic.h"
#include "aptima_utils/lib/mutex.h"

typedef struct aptima_transportbackend_t aptima_transportbackend_t;
typedef struct aptima_string_t aptima_string_t;
typedef struct aptima_stream_t aptima_stream_t;

typedef enum aptima_TRANSPORT_DROP_TYPE {
  /* Drop oldest data when transport channel is full, only available when
     transport type is shm or raw pointer */
  aptima_TRANSPORT_DROP_OLD,

  /* Drop current data when transport channel is full */
  aptima_TRANSPORT_DROP_NEW,

  /* Drop current data if no reader is waiting */
  aptima_TRANSPORT_DROP_IF_NO_READER,

  /* Drop current data by asking, only available when
     transport type is shm or raw pointer .
     Useful if user wan't to drop by bussiness logic (e.g. never drop I frame)
   */
  aptima_TRANSPORT_DROP_ASK
} aptima_TRANSPORT_DROP_TYPE;

typedef struct aptima_transport_t aptima_transport_t;

struct aptima_transport_t {
  /**
   * uv loop that attached to current thread
   */
  aptima_runloop_t *loop;

  void *user_data;

  aptima_transportbackend_t *backend;
  aptima_atomic_t close;

  aptima_mutex_t *lock;
  int drop_when_full;
  aptima_TRANSPORT_DROP_TYPE drop_type;

  /**
   * Callback when a new rx stream is connected
   */
  void (*on_server_connected)(aptima_transport_t *transport, aptima_stream_t *stream,
                              int status);
  void *on_server_connected_data;

  /**
   * Callback when a new rx stream is created
   */
  void (*on_client_accepted)(aptima_transport_t *transport, aptima_stream_t *stream,
                             int status);
  void *on_client_accepted_data;

  /**
   * Callback when transport closed
   */
  void (*on_closed)(void *on_closed_data);
  void *on_closed_data;
};

// Public interface
aptima_UTILS_API aptima_transport_t *aptima_transport_create(aptima_runloop_t *loop);

aptima_UTILS_API int aptima_transport_close(aptima_transport_t *self);

aptima_UTILS_API void aptima_transport_set_close_cb(aptima_transport_t *self,
                                              void *close_cb,
                                              void *close_cb_data);

aptima_UTILS_API int aptima_transport_listen(aptima_transport_t *self,
                                       const aptima_string_t *my_uri);

aptima_UTILS_API int aptima_transport_connect(aptima_transport_t *self,
                                        aptima_string_t *dest);
