//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include "aptima_utils/io/runloop.h"

typedef struct aptima_shm_channel_t aptima_shm_channel_t;

aptima_UTILS_API int aptima_shm_channel_create(const char *name,
                                         aptima_shm_channel_t *channel[2]);

aptima_UTILS_API void aptima_shm_channel_close(aptima_shm_channel_t *channel);

aptima_UTILS_API int aptima_shm_channel_active(aptima_shm_channel_t *channel, int read);

aptima_UTILS_API int aptima_shm_channel_inactive(aptima_shm_channel_t *channel,
                                           int read);

aptima_UTILS_API int aptima_shm_channel_wait_remote(aptima_shm_channel_t *channel,
                                              int wait_ms);

aptima_UTILS_API int aptima_shm_channel_send(aptima_shm_channel_t *channel, void *data,
                                       size_t size, int nonblock);

aptima_UTILS_API int aptima_shm_channel_recv(aptima_shm_channel_t *channel, void *data,
                                       size_t size, int nonblock);

aptima_UTILS_API int aptima_shm_channel_get_capacity(aptima_shm_channel_t *channel);

aptima_UTILS_API int aptima_shm_channel_set_signal(aptima_shm_channel_t *channel,
                                             aptima_runloop_async_t *signal,
                                             int read);
