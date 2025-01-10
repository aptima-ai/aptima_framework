//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/shmchannel.h"

#include <stdlib.h>
#include <string.h>

#include "axis_utils/lib/shared_event.h"
#include "axis_utils/lib/shm.h"
#include "axis_utils/lib/spinlock.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/mark.h"

#if defined(_WIN32)
#define axis_ANYSIZE_ARRAY 1
#else
#define axis_ANYSIZE_ARRAY 0
#endif

typedef struct axis_shm_layout_t {
  axis_atomic_t id;
  axis_atomic_t ref_count;
  axis_atomic_t read_index;
  axis_atomic_t write_index;
  axis_atomic_t channel_lock;
  struct {
    uint32_t sig;
    uint32_t dummy;
    axis_atomic_t lock;
  } reader_active, writer_active, not_full, not_empty;

  uint8_t data[axis_ANYSIZE_ARRAY];
} axis_shm_layout_t;

typedef struct axis_shm_channel_t {
  axis_shm_layout_t *region;
  axis_string_t name;
  axis_atomic_t active;
  int as_reader;
  axis_spinlock_t *channel_lock;
  axis_shared_event_t *reader_active;
  axis_shared_event_t *writer_active;
  axis_shared_event_t *not_full;
  axis_shared_event_t *not_empty;
  axis_runloop_async_t *read_sig;
  axis_runloop_async_t *write_sig;
} axis_shm_channel_t;

#define axis_SHM_MEM_SIZE (1 * 1024 * 1024)
#define axis_SHM_CHANNEL_SIZE (axis_SHM_MEM_SIZE - sizeof(axis_shm_layout_t))
#define axis_SHM_NAME_FORMAT "%s_%d"

int axis_shm_channel_create(const char *name, axis_shm_channel_t *channel[2]) {
  if (UNLIKELY(!name || !*name || !channel)) {
    return -1;
  }

  for (int i = 0; i < 2; i++) {
    channel[i] = (axis_shm_channel_t *)malloc(sizeof(axis_shm_channel_t));
    assert(channel[i]);
    memset(channel[i], 0, sizeof(axis_shm_channel_t));

    axis_string_init_formatted(&channel[i]->name, axis_SHM_NAME_FORMAT, name, i);

    channel[i]->region =
        (axis_shm_layout_t *)axis_shm_map(channel[i]->name.buf, axis_SHM_MEM_SIZE);
    assert(channel[i]->region);
    axis_atomic_store(&channel[i]->region->id, i);

    axis_atomic_add_fetch(&channel[i]->region->ref_count, 1);

    channel[i]->channel_lock =
        axis_spinlock_from_addr(&channel[i]->region->channel_lock);
    assert(channel[i]->channel_lock);

    channel[i]->reader_active =
        axis_shared_event_create(&channel[i]->region->reader_active.sig,
                                &channel[i]->region->reader_active.lock, 0, 0);
    assert(channel[i]->reader_active);

    channel[i]->writer_active =
        axis_shared_event_create(&channel[i]->region->writer_active.sig,
                                &channel[i]->region->writer_active.lock, 0, 0);
    assert(channel[i]->writer_active);

    channel[i]->not_full =
        axis_shared_event_create(&channel[i]->region->not_full.sig,
                                &channel[i]->region->not_full.lock, 0, 1);
    assert(channel[i]->not_full);

    channel[i]->not_empty =
        axis_shared_event_create(&channel[i]->region->not_empty.sig,
                                &channel[i]->region->not_empty.lock, 0, 1);
    assert(channel[i]->not_empty);
  }

  return 0;
}

void axis_shm_channel_close(axis_shm_channel_t *channel) {
  if (UNLIKELY(!channel || !channel->region)) {
    return;
  }

  void *region = channel->region;
  int64_t ref_count = axis_atomic_fetch_sub(&channel->region->ref_count, 1);
  int64_t id = axis_atomic_load(&channel->region->id);

  if (axis_atomic_load(&channel->active)) {
    axis_shm_channel_inactive(channel, channel->as_reader);
  }

  if (channel->reader_active) {
    axis_shared_event_destroy(channel->reader_active);
    channel->reader_active = NULL;
  }

  if (channel->writer_active) {
    axis_shared_event_destroy(channel->writer_active);
    channel->writer_active = NULL;
  }

  if (channel->not_full) {
    axis_shared_event_destroy(channel->not_full);
    channel->not_full = NULL;
  }

  if (channel->not_empty) {
    axis_shared_event_destroy(channel->not_empty);
    channel->not_empty = NULL;
  }

  axis_shm_unmap(region);

  if (ref_count == 1) {
    axis_shm_unlink(channel->name.buf);
  }

  free(channel);
}

int axis_shm_channel_active(axis_shm_channel_t *channel, int read) {
  if (UNLIKELY(!channel || !channel->region)) {
    return -1;
  }

  axis_atomic_store(&channel->active, 1);
  channel->as_reader = read;
  if (read) {
    axis_shared_event_set(channel->reader_active);
  } else {
    axis_shared_event_set(channel->writer_active);
  }

  return 0;
}

int axis_shm_channel_inactive(axis_shm_channel_t *channel, int read) {
  if (UNLIKELY(!channel || !channel->region)) {
    return -1;
  }

  if (!axis_atomic_load(&channel->active)) {
    return -1;
  }

  if (read) {
    axis_shared_event_reset(channel->reader_active);
    axis_shared_event_set(channel->not_full);
  } else {
    axis_shared_event_reset(channel->writer_active);
    axis_shared_event_set(channel->not_empty);
  }

  axis_atomic_store(&channel->active, 0);

  return 0;
}

static inline int __axis_shm_channel_get_capacity_unsafe(
    axis_shm_channel_t *channel) {
  return (int)((channel->region->write_index + axis_SHM_CHANNEL_SIZE -
                channel->region->read_index) %
               axis_SHM_CHANNEL_SIZE);
}

static inline int __axis_shm_channel_is_full_unsafe(axis_shm_channel_t *channel) {
  return __axis_shm_channel_get_capacity_unsafe(channel) ==
         (axis_SHM_CHANNEL_SIZE - 1);
}

static inline int __axis_shm_channel_is_empty_unsafe(
    axis_shm_channel_t *channel) {
  return __axis_shm_channel_get_capacity_unsafe(channel) == 0;
}

static inline int __axis_shm_channel_reader_alive(axis_shm_channel_t *channel) {
  return axis_shared_event_wait(channel->reader_active, 0) == 0;
}

static inline int __axis_shm_channel_writer_alive(axis_shm_channel_t *channel) {
  return axis_shared_event_wait(channel->writer_active, 0) == 0;
}

int axis_shm_channel_send(axis_shm_channel_t *channel, void *data, size_t size,
                         int nonblock) {
  if (UNLIKELY(!channel || !channel->region || !data || !size)) {
    return -1;
  }

  if (!axis_atomic_load(&channel->active) || channel->as_reader != 0) {
    return -1;
  }

  size_t left = size;
  while (left) {
    if (!__axis_shm_channel_reader_alive(channel)) {
      return -1;
    }

    axis_spinlock_lock(channel->channel_lock);

    if (__axis_shm_channel_is_full_unsafe(channel)) {
      axis_spinlock_unlock(channel->channel_lock);

      if (nonblock) {
        if (left != size && channel->read_sig) {
          axis_runloop_async_notify(channel->read_sig);
        }

        return (size - left);
      }

      axis_shared_event_wait(channel->not_full, -1);
      axis_spinlock_lock(channel->channel_lock);
    }

    if (!__axis_shm_channel_reader_alive(channel)) {
      axis_spinlock_unlock(channel->channel_lock);
      return -1;
    }

    int caps = axis_SHM_CHANNEL_SIZE -
               __axis_shm_channel_get_capacity_unsafe(channel) - 1;

    size_t copy_size = left > caps ? caps : left;
    size_t first = axis_SHM_CHANNEL_SIZE - channel->region->write_index;
    size_t second = copy_size - first;

    size_t copy_left = first > copy_size ? copy_size : first;
    if (copy_left) {
      axis_spinlock_unlock(channel->channel_lock);
      memmove(channel->region->data + channel->region->write_index, data,
              copy_left);
      axis_spinlock_lock(channel->channel_lock);
      channel->region->write_index += copy_left;
      channel->region->write_index %= axis_SHM_CHANNEL_SIZE;
      left -= copy_left;
      // printf("w[1] cap %d, %d\n", caps, copy_left);
    }

    if (copy_size > copy_left && second) {
      axis_spinlock_unlock(channel->channel_lock);
      memmove(channel->region->data, (char *)data + copy_left, second);
      axis_spinlock_lock(channel->channel_lock);
      channel->region->write_index = second;
      channel->region->write_index %= axis_SHM_CHANNEL_SIZE;
      left -= second;
      // printf("w[2] cap %d, %d\n", caps, second);
    }

    axis_spinlock_unlock(channel->channel_lock);

    axis_shared_event_set(channel->not_empty);
  }

  if (nonblock && channel->read_sig) {
    axis_runloop_async_notify(channel->read_sig);
  }

  return (int)size;
}

int axis_shm_channel_recv(axis_shm_channel_t *channel, void *data, size_t size,
                         int nonblock) {
  if (UNLIKELY(!channel || !channel->region || !data || !size)) {
    return -1;
  }

  if (!axis_atomic_load(&channel->active) || channel->as_reader != 1) {
    return -1;
  }

  size_t left = size;
  while (left) {
    if (!__axis_shm_channel_writer_alive(channel)) {
      return -1;
    }

    axis_spinlock_lock(channel->channel_lock);

    if (__axis_shm_channel_is_empty_unsafe(channel)) {
      axis_spinlock_unlock(channel->channel_lock);

      if (nonblock) {
        if (left != size && channel->write_sig) {
          axis_runloop_async_notify(channel->write_sig);
        }

        return (size - left);
      }

      axis_shared_event_wait(channel->not_empty, -1);
      axis_spinlock_lock(channel->channel_lock);
    }

    if (!__axis_shm_channel_writer_alive(channel)) {
      axis_spinlock_unlock(channel->channel_lock);
      return -1;
    }

    int caps = __axis_shm_channel_get_capacity_unsafe(channel);

    size_t copy_size = left > caps ? caps : left;
    size_t first = axis_SHM_CHANNEL_SIZE - channel->region->read_index;
    size_t second = copy_size - first;

    size_t copy_left = first > copy_size ? copy_size : first;
    if (copy_left) {
      axis_spinlock_unlock(channel->channel_lock);
      memmove(data, channel->region->data + channel->region->read_index,
              copy_left);
      axis_spinlock_lock(channel->channel_lock);
      channel->region->read_index += copy_left;
      channel->region->read_index %= axis_SHM_CHANNEL_SIZE;
      left -= copy_left;
      // printf("r[1] cap %d, %d\n", caps, copy_left);
    }

    if (copy_size > copy_left && second) {
      axis_spinlock_unlock(channel->channel_lock);
      memmove((char *)data + first, channel->region->data, second);
      axis_spinlock_lock(channel->channel_lock);
      channel->region->read_index = second;
      channel->region->read_index %= axis_SHM_CHANNEL_SIZE;
      left -= second;
      // printf("r[2] cap %d, %d\n", caps, second);
    }

    axis_spinlock_unlock(channel->channel_lock);

    axis_shared_event_set(channel->not_full);
  }

  if (nonblock && channel->write_sig) {
    axis_runloop_async_notify(channel->write_sig);
  }

  return (int)size;
}

int axis_shm_channel_get_capacity(axis_shm_channel_t *channel) {
  if (UNLIKELY(!channel || !channel->channel_lock)) {
    return -1;
  }

  axis_spinlock_lock(channel->channel_lock);
  int diff = __axis_shm_channel_get_capacity_unsafe(channel);
  axis_spinlock_unlock(channel->channel_lock);

  return diff;
}

int axis_shm_channel_set_signal(axis_shm_channel_t *channel,
                               axis_runloop_async_t *signal, int read) {
  axis_memory_barrier();
  if (read) {
    channel->read_sig = signal;
  } else {
    channel->write_sig = signal;
  }
  axis_memory_barrier();
  return 0;
}

int axis_shm_channel_wait_remote(axis_shm_channel_t *channel, int wait_ms) {
  if (!channel || !channel->region) {
    return -1;
  }

  if (!axis_atomic_load(&channel->active)) {
    return -1;
  }

  return channel->as_reader
             ? axis_shared_event_wait(channel->writer_active, wait_ms)
             : axis_shared_event_wait(channel->reader_active, wait_ms);
}
