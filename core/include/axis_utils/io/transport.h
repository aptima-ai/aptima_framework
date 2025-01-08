//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/mutex.h"

typedef struct axis_transportbackend_t axis_transportbackend_t;
typedef struct axis_string_t axis_string_t;
typedef struct axis_stream_t axis_stream_t;

typedef enum axis_TRANSPORT_DROP_TYPE {
  /* Drop oldest data when transport channel is full, only available when
     transport type is shm or raw pointer */
  axis_TRANSPORT_DROP_OLD,

  /* Drop current data when transport channel is full */
  axis_TRANSPORT_DROP_NEW,

  /* Drop current data if no reader is waiting */
  axis_TRANSPORT_DROP_IF_NO_READER,

  /* Drop current data by asking, only available when
     transport type is shm or raw pointer .
     Useful if user wan't to drop by bussiness logic (e.g. never drop I frame)
   */
  axis_TRANSPORT_DROP_ASK
} axis_TRANSPORT_DROP_TYPE;

typedef struct axis_transport_t axis_transport_t;

struct axis_transport_t {
  /**
   * uv loop that attached to current thread
   */
  axis_runloop_t *loop;

  void *user_data;

  axis_transportbackend_t *backend;
  axis_atomic_t close;

  axis_mutex_t *lock;
  int drop_when_full;
  axis_TRANSPORT_DROP_TYPE drop_type;

  /**
   * Callback when a new rx stream is connected
   */
  void (*on_server_connected)(axis_transport_t *transport, axis_stream_t *stream,
                              int status);
  void *on_server_connected_data;

  /**
   * Callback when a new rx stream is created
   */
  void (*on_client_accepted)(axis_transport_t *transport, axis_stream_t *stream,
                             int status);
  void *on_client_accepted_data;

  /**
   * Callback when transport closed
   */
  void (*on_closed)(void *on_closed_data);
  void *on_closed_data;
};

// Public interface
axis_UTILS_API axis_transport_t *axis_transport_create(axis_runloop_t *loop);

axis_UTILS_API int axis_transport_close(axis_transport_t *self);

axis_UTILS_API void axis_transport_set_close_cb(axis_transport_t *self,
                                              void *close_cb,
                                              void *close_cb_data);

axis_UTILS_API int axis_transport_listen(axis_transport_t *self,
                                       const axis_string_t *my_uri);

axis_UTILS_API int axis_transport_connect(axis_transport_t *self,
                                        axis_string_t *dest);
