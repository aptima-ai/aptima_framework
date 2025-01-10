//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stddef.h>

#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/string.h"

typedef struct axis_transport_t axis_transport_t;
typedef struct axis_transportbackend_t axis_transportbackend_t;

struct axis_transportbackend_t {
  axis_atomic_t is_close;
  const char *impl;
  axis_string_t *name;
  axis_transport_t *transport;

  int (*connect)(axis_transportbackend_t *backend, const axis_string_t *dest);
  int (*listen)(axis_transportbackend_t *backend, const axis_string_t *dest);
  void (*close)(axis_transportbackend_t *backend);
};

typedef struct axis_transportbackend_factory_t {
  axis_transportbackend_t *(*create)(axis_transport_t *transport,
                                    const axis_string_t *name);
} axis_transportbackend_factory_t;

axis_UTILS_PRIVATE_API void axis_transportbackend_init(
    axis_transportbackend_t *self, axis_transport_t *transport,
    const axis_string_t *name);

axis_UTILS_PRIVATE_API void axis_transportbackend_deinit(
    axis_transportbackend_t *self);

typedef struct axis_stream_t axis_stream_t;
typedef struct axis_streambackend_t axis_streambackend_t;

struct axis_streambackend_t {
  axis_atomic_t is_close;
  axis_stream_t *stream;
  const char *impl;

  int (*start_read)(axis_streambackend_t *self);
  int (*stop_read)(axis_streambackend_t *self);

  int (*write)(axis_streambackend_t *self, const void *buf, size_t size,
               void *user_data);

  int (*close)(axis_streambackend_t *self);
};

axis_UTILS_PRIVATE_API void axis_streambackend_init(const char *impl,
                                                  axis_streambackend_t *backend,
                                                  axis_stream_t *stream);

axis_UTILS_PRIVATE_API void axis_streambackend_deinit(
    axis_streambackend_t *backend);
