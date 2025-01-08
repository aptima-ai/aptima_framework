//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <uv.h>

#include "axis_utils/io/stream.h"

typedef struct axis_migrate_t {
  axis_stream_t *stream;

  axis_runloop_t *from;
  axis_runloop_t *to;

#if !defined(_WIN32)
  uv_os_sock_t fds[2];
#else
  uv_file fds[2];
#endif
  uv_pipe_t *pipe[2];

  int migrate_processed;

  axis_atomic_t expect_finalize_count;
  axis_atomic_t finalized_count;

  // @{
  // The following 2 'async' belong to the 'from' thread/runloop.
  uv_async_t src_prepare;
  uv_async_t src_migration;
  // @}

  // @{
  // The following 2 'async' belong to the 'to' thread/runloop.
  uv_async_t dst_prepare;
  uv_async_t dst_migration;
  // @}

  void **user_data;
  void (*migrated)(axis_stream_t *new_stream, void **user_data);
} axis_migrate_t;

axis_UTILS_PRIVATE_API void migration_dst_prepare(uv_async_t *async);

axis_UTILS_PRIVATE_API void migration_dst_start(uv_async_t *async);

axis_UTILS_PRIVATE_API int axis_stream_migrate_uv_stage2(axis_migrate_t *migrate);
