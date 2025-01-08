//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <uv.h>

#include "axis_utils/io/general/transport/backend/base.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/atomic.h"

#define axis_STREAMBACKEND_TCP_SIGNATURE 0x861D0758EA843914U

typedef struct axis_transport_t axis_transport_t;

typedef struct axis_streambackend_tcp_t {
  axis_streambackend_t base;

  axis_atomic_t signature;

  uv_stream_t *uv_stream;
} axis_streambackend_tcp_t;

axis_UTILS_PRIVATE_API axis_stream_t *axis_stream_tcp_create_uv(uv_loop_t *loop);

axis_UTILS_PRIVATE_API void axis_streambackend_tcp_dump_info(
    axis_streambackend_tcp_t *tcp_stream, const char *fmt, ...);
