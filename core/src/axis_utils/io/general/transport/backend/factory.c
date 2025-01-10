//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/general/transport/backend/factory.h"

#include <string.h>

#include "include_internal/axis_utils/io/runloop.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/io/stream.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/uri.h"
#include "axis_utils/log/log.h"

typedef struct axis_backend_map_t {
  const char *name;
  const axis_transportbackend_factory_t *factory;
} axis_backend_map_t;

typedef struct axis_factory_map_t {
  const char *name;
  const axis_backend_map_t *factory;
  const size_t size;
} axis_factory_map_t;

extern const axis_transportbackend_factory_t general_tp_backend_raw;

#if defined(axis_USE_LIBUV)

extern const axis_transportbackend_factory_t uv_tp_backend_tcp;
extern const axis_transportbackend_factory_t uv_tp_backend_pipe;

static const axis_backend_map_t uv_backend_map[] = {
    {axis_PROTOCOL_TCP, &uv_tp_backend_tcp},
    {axis_PROTOCOL_RAW, &general_tp_backend_raw},
    {axis_PROTOCOL_PIPE, &uv_tp_backend_pipe},
};

#endif

#if defined(axis_USE_LIBEVENT)

extern const axis_transportbackend_factory_t event_tp_backend_tcp;
extern const axis_transportbackend_factory_t event_tp_backend_pipe;

static const axis_backend_map_t event_backend_map[] = {
    {axis_PROTOCOL_TCP, &event_tp_backend_tcp},
    {axis_PROTOCOL_RAW, &general_tp_backend_raw},
    {axis_PROTOCOL_PIPE, &event_tp_backend_pipe},
};

#endif

static const axis_factory_map_t factory_map[] = {
#if defined(axis_USE_LIBEVENT)
    {axis_RUNLOOP_EVENT2, event_backend_map,
     sizeof(event_backend_map) / sizeof(event_backend_map[0])},
#endif

#if defined(axis_USE_LIBUV)
    {axis_RUNLOOP_UV, uv_backend_map,
     sizeof(uv_backend_map) / sizeof(uv_backend_map[0])},
#endif
};

axis_transportbackend_factory_t *axis_get_transportbackend_factory(
    const char *choice, const axis_string_t *uri) {
  const axis_factory_map_t *map = NULL;
  const size_t map_size = 0;

  for (size_t i = 0; i < sizeof(factory_map) / sizeof(axis_factory_map_t); i++) {
    if (strcmp(factory_map[i].name, choice) == 0) {
      map = &factory_map[i];
      break;
    }
  }

  if (map == NULL) {
    return NULL;
  }

  axis_string_t *protocol = axis_uri_get_protocol(axis_string_get_raw_str(uri));

  for (size_t i = 0; i < map->size; i++) {
    if (strcmp(map->factory[i].name, protocol->buf) == 0 &&
        map->factory[i].factory != NULL &&
        map->factory[i].factory->create != NULL) {
      axis_string_destroy(protocol);
      return (axis_transportbackend_factory_t *)map->factory[i].factory;
    }
  }

  axis_string_destroy(protocol);
  return NULL;
}

#if defined(axis_USE_LIBUV)
extern int axis_stream_migrate_uv(axis_stream_t *, axis_runloop_t *,
                                 axis_runloop_t *, void **,
                                 void (*)(axis_stream_t *, void **));
#endif

#if defined(axis_USE_LIBEVENT)
extern int axis_stream_migrate_ev(axis_stream_t *, axis_runloop_t *,
                                 axis_runloop_t *, void **,
                                 void (*)(axis_stream_t *, void **));
#endif

int axis_stream_migrate(axis_stream_t *self, axis_runloop_t *from,
                       axis_runloop_t *to, void **user_data,
                       void (*cb)(axis_stream_t *new_stream, void **user_data)) {
  if (!self || !from || !to) {
    axis_LOGE("Invalid parameter, self %p, from %p, to %p", self, from, to);
    return -1;
  }

  if (strcmp(from->impl, to->impl) != 0) {
    return -1;
  }

#if defined(axis_USE_LIBUV)
  if (strcmp(from->impl, axis_RUNLOOP_UV) == 0) {
    return axis_stream_migrate_uv(self, from, to, user_data, cb);
  }
#endif

#if defined(axis_USE_LIBEVENT)
  if (strcmp(from->impl, axis_RUNLOOP_EVENT2) == 0) {
    return axis_stream_migrate_ev(self, from, to, user_data, cb);
  }
#endif

  return -1;
}
