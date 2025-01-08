//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/io/runloop.h"

typedef struct axis_shm_channel_t axis_shm_channel_t;

axis_UTILS_API int axis_shm_channel_create(const char *name,
                                         axis_shm_channel_t *channel[2]);

axis_UTILS_API void axis_shm_channel_close(axis_shm_channel_t *channel);

axis_UTILS_API int axis_shm_channel_active(axis_shm_channel_t *channel, int read);

axis_UTILS_API int axis_shm_channel_inactive(axis_shm_channel_t *channel,
                                           int read);

axis_UTILS_API int axis_shm_channel_wait_remote(axis_shm_channel_t *channel,
                                              int wait_ms);

axis_UTILS_API int axis_shm_channel_send(axis_shm_channel_t *channel, void *data,
                                       size_t size, int nonblock);

axis_UTILS_API int axis_shm_channel_recv(axis_shm_channel_t *channel, void *data,
                                       size_t size, int nonblock);

axis_UTILS_API int axis_shm_channel_get_capacity(axis_shm_channel_t *channel);

axis_UTILS_API int axis_shm_channel_set_signal(axis_shm_channel_t *channel,
                                             axis_runloop_async_t *signal,
                                             int read);
