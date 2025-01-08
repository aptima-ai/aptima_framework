//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once
#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/signature.h"

#define axis_STREAM_SIGNATURE 0xDE552052E7F8EE10U
#define axis_STREAM_DEFAULT_BUF_SIZE (64 * 1024)

typedef struct axis_stream_t axis_stream_t;
typedef struct axis_transport_t axis_transport_t;
typedef struct axis_streambackend_t axis_streambackend_t;

struct axis_stream_t {
  axis_signature_t signature;
  axis_atomic_t close;

  axis_transport_t *transport;
  axis_streambackend_t *backend;

  void *user_data;

  void (*on_message_read)(axis_stream_t *stream, void *msg, int size);
  void (*on_message_sent)(axis_stream_t *stream, int status, void *args);
  void (*on_message_free)(axis_stream_t *stream, int status, void *args);

  void (*on_closed)(void *on_closed_data);
  void *on_closed_data;
};

// Public interface
/**
 * @brief Begin read from stream.
 * @param self The stream to read from.
 * @return 0 if success, otherwise -1.
 */
axis_UTILS_API int axis_stream_start_read(axis_stream_t *self);

/**
 * @brief Stop read from stream.
 * @param self The stream to read from.
 * @return 0 if success, otherwise -1.
 */
axis_UTILS_API int axis_stream_stop_read(axis_stream_t *self);

/**
 * @brief Send a message to stream.
 * @param self The stream to send to.
 * @param msg The message to send.
 * @param size The size of message.
 * @return 0 if success, otherwise -1.
 */
axis_UTILS_API int axis_stream_send(axis_stream_t *self, const char *msg,
                                  uint32_t size, void *user_data);

/**
 * @brief Close the stream.
 * @param self The stream to close.
 */
axis_UTILS_API void axis_stream_close(axis_stream_t *self);

/**
 * @brief Set close callback for stream.
 * @param self The stream to set close callback for.
 * @param close_cb The callback to set.
 * @param close_cb_data The args of |close_cb| when it's been called
 */
axis_UTILS_API void axis_stream_set_on_closed(axis_stream_t *self, void *on_closed,
                                            void *on_closed_data);

/**
 * @brief Migrate a stream from one runloop to another.
 *
 * @param self The stream to migrate
 * @param from The runloop to migrate from
 * @param to The runloop to migrate to
 * @param user_data The user data to be passed to the callback
 * @param cb The callback to be called when the migration is done
 *
 * @return 0 on success, -1 on failure
 *
 * @note 1. |cb| will be called in |to| loop thread if success
 *       2. |from| and |to| have to be the same implementation type
 *          (e.g.event2, uv, etc.)
 *       3. |self| will be removed from |from| loop and no more data
 *          will be read from it
 */
axis_UTILS_API int axis_stream_migrate(axis_stream_t *self, axis_runloop_t *from,
                                     axis_runloop_t *to, void **user_data,
                                     void (*cb)(axis_stream_t *new_stream,
                                                void **user_data));

axis_UTILS_API bool axis_stream_check_integrity(axis_stream_t *self);

axis_UTILS_API void axis_stream_init(axis_stream_t *self);

axis_UTILS_API void axis_stream_on_close(axis_stream_t *self);
