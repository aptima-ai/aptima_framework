//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/sanitizer/memory_check.h"

#include "include_internal/axis_utils/lib/alloc.h"

#if defined(axis_USE_ASAN)
#include <sanitizer/asan_interface.h>
#include <sanitizer/lsan_interface.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "include_internal/axis_utils/sanitizer/memory_check.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

// Note: Since TEN LOG also involves memory operations, to avoid circular
// dependencies in the memory checker system, use the basic printf family
// functions instead of TEN LOG.

// Note: The `axis_sanitizer_memory_record_t` items stored in
// `axis_sanitizer_memory_records_t` not only contain the actual allocated memory
// region addresses but also include other auxiliary allocations used to record
// these memory regions in `axis_sanitizer_memory_records_t`. However, these
// auxiliary allocations are only for recording the actual allocated memory
// region addresses and do not need to be tracked by LeakSanitizer. Otherwise,
// in addition to the actual memory leaks, each actual memory leak would have
// corresponding internal auxiliary allocations used for recording, which would
// essentially be false alarms. Therefore, `__lsan_disable()` and
// `__lsan_enable()` are used to exclude these auxiliary allocations from being
// tracked by LeakSanitizer.

static axis_sanitizer_memory_records_t g_memory_records = {NULL,
                                                          axis_LIST_INIT_VAL, 0};
static bool g_memory_records_enabled = false;

static void axis_sanitizer_memory_record_check_enabled(void) {
#if defined(axis_ENABLE_MEMORY_CHECK)
  char *enable_memory_sanitizer = getenv("axis_ENABLE_MEMORY_TRACKING");
  if (enable_memory_sanitizer && !strcmp(enable_memory_sanitizer, "true")) {
    g_memory_records_enabled = true;
  }
#endif
}

void axis_sanitizer_memory_record_init(void) {
#if defined(axis_ENABLE_MEMORY_CHECK)

#if defined(axis_USE_ASAN)
  __lsan_disable();
#endif

  axis_sanitizer_memory_record_check_enabled();

#if defined(axis_USE_ASAN)
  // Mark the beginning and end of the globally allocated memory queue as
  // poisoned, so that LSan will not consider the memory buffer obtained from
  // there as normal memory, but will instead consider it as leaked.
  __asan_poison_memory_region(&g_memory_records.records.front,
                              sizeof(axis_listnode_t *));
  __asan_poison_memory_region(&g_memory_records.records.back,
                              sizeof(axis_listnode_t *));
#endif

  g_memory_records.lock = axis_mutex_create();

#if defined(axis_USE_ASAN)
  __lsan_enable();
#endif

#endif
}

void axis_sanitizer_memory_record_deinit(void) {
#if defined(axis_ENABLE_MEMORY_CHECK)

#if defined(axis_USE_ASAN)
  __lsan_disable();
#endif

  axis_sanitizer_memory_record_dump();

  if (g_memory_records.lock) {
    axis_mutex_destroy(g_memory_records.lock);
  }

#if defined(axis_USE_ASAN)
  __lsan_enable();
#endif

#endif
}

static axis_sanitizer_memory_record_t *axis_sanitizer_memory_record_create(
    void *addr, size_t size, const char *file_name, uint32_t lineno,
    const char *func_name) {
#if defined(axis_USE_ASAN)
  __lsan_disable();
#endif

  axis_sanitizer_memory_record_t *self =
      malloc(sizeof(axis_sanitizer_memory_record_t));
  axis_ASSERT(self, "Failed to allocate memory.");
  if (!self) {
#if defined(axis_USE_ASAN)
    __lsan_enable();
#endif
    return NULL;
  }

  self->addr = addr;
  self->size = size;

  self->func_name = (char *)malloc(strlen(func_name) + 1);
  axis_ASSERT(self->func_name, "Failed to allocate memory.");

  int written =
      snprintf(self->func_name, strlen(func_name) + 1, "%s", func_name);
  axis_ASSERT(written > 0, "Should not happen.");

  self->file_name = (char *)malloc(strlen(file_name) + 1);
  axis_ASSERT(self->file_name, "Failed to allocate memory.");
  axis_ASSERT(strlen(file_name) >= axis_FILE_PATH_RELATIVE_PREFIX_LENGTH,
             "Should not happen.");

  written =
      snprintf(self->file_name, strlen(file_name) + 1, "%.*s",
               (int)(strlen(file_name) - axis_FILE_PATH_RELATIVE_PREFIX_LENGTH),
               file_name + axis_FILE_PATH_RELATIVE_PREFIX_LENGTH);
  axis_ASSERT(written > 0, "Should not happen.");

  self->lineno = lineno;

#if defined(axis_USE_ASAN)
  __lsan_enable();
#endif

  return self;
}

static void axis_sanitizer_memory_record_destroy(
    axis_sanitizer_memory_record_t *self) {
#if defined(axis_USE_ASAN)
  __lsan_disable();
#endif

  axis_ASSERT(self, "Invalid argument.");

  free(self->func_name);
  free(self->file_name);
  free(self);

#if defined(axis_USE_ASAN)
  __lsan_enable();
#endif
}

static void axis_sanitizer_memory_record_add(
    axis_sanitizer_memory_records_t *self,
    axis_sanitizer_memory_record_t *record) {
#if defined(axis_USE_ASAN)
  __lsan_disable();
#endif

  axis_ASSERT(self && record, "Invalid argument.");

  axis_UNUSED int rc = axis_mutex_lock(self->lock);
  axis_ASSERT(!rc, "Failed to lock.");

#if defined(axis_USE_ASAN)
  __asan_unpoison_memory_region(&self->records.front, sizeof(axis_listnode_t *));
  __asan_unpoison_memory_region(&self->records.back, sizeof(axis_listnode_t *));
#endif

  axis_list_push_ptr_back(
      &self->records, record,
      (axis_ptr_listnode_destroy_func_t)axis_sanitizer_memory_record_destroy);

#if defined(axis_USE_ASAN)
  __asan_poison_memory_region(
      (((axis_ptr_listnode_t *)(self->records.back))->ptr),
      sizeof(axis_sanitizer_memory_record_t *));

  __asan_poison_memory_region(&self->records.front, sizeof(axis_listnode_t *));
  __asan_poison_memory_region(&self->records.back, sizeof(axis_listnode_t *));
#endif

  self->total_size += record->size;

  rc = axis_mutex_unlock(self->lock);
  axis_ASSERT(!rc, "Failed to unlock.");

#if defined(axis_USE_ASAN)
  __lsan_enable();
#endif
}

static void axis_sanitizer_memory_record_del(
    axis_sanitizer_memory_records_t *self, void *addr) {
#if defined(axis_USE_ASAN)
  __lsan_disable();
#endif

  axis_ASSERT(self && addr, "Invalid argument.");

  axis_UNUSED int rc = axis_mutex_lock(self->lock);
  axis_ASSERT(!rc, "Failed to lock.");

#if defined(axis_USE_ASAN)
  __asan_unpoison_memory_region(&self->records.front, sizeof(axis_listnode_t *));
  __asan_unpoison_memory_region(&self->records.back, sizeof(axis_listnode_t *));
#endif

  axis_list_foreach (&self->records, iter) {
#if defined(axis_USE_ASAN)
    __asan_unpoison_memory_region((((axis_ptr_listnode_t *)(iter.node))->ptr),
                                  sizeof(axis_sanitizer_memory_record_t *));
#endif

    axis_sanitizer_memory_record_t *record = axis_ptr_listnode_get(iter.node);

    if (record->addr == addr) {
      axis_ASSERT(self->total_size >= record->size, "Should not happen.");
      self->total_size -= record->size;
      axis_list_remove_node(&self->records, iter.node);
      break;
    }

#if defined(axis_USE_ASAN)
    __asan_poison_memory_region((((axis_ptr_listnode_t *)(iter.node))->ptr),
                                sizeof(axis_sanitizer_memory_record_t *));
#endif
  }

#if defined(axis_USE_ASAN)
  __asan_poison_memory_region(&self->records.front, sizeof(axis_listnode_t *));
  __asan_poison_memory_region(&self->records.back, sizeof(axis_listnode_t *));
#endif

  rc = axis_mutex_unlock(self->lock);
  axis_ASSERT(!rc, "Failed to unlock.");

#if defined(axis_USE_ASAN)
  __lsan_enable();
#endif
}

void axis_sanitizer_memory_record_dump(void) {
#if defined(axis_ENABLE_MEMORY_CHECK)

#if defined(axis_USE_ASAN)
  __lsan_disable();
#endif

  axis_UNUSED int rc = axis_mutex_lock(g_memory_records.lock);
  axis_ASSERT(!rc, "Failed to lock.");

  if (g_memory_records.total_size) {
    (void)fprintf(stderr, "Memory allocation summary(%zu bytes):\n",
                  g_memory_records.total_size);
  }

#if defined(axis_USE_ASAN)
  __asan_unpoison_memory_region(&g_memory_records.records.front,
                                sizeof(axis_listnode_t *));
  __asan_unpoison_memory_region(&g_memory_records.records.back,
                                sizeof(axis_listnode_t *));
#endif

  size_t idx = 0;
  axis_list_foreach (&g_memory_records.records, iter) {
#if defined(axis_USE_ASAN)
    __asan_unpoison_memory_region((((axis_ptr_listnode_t *)(iter.node))->ptr),
                                  sizeof(axis_sanitizer_memory_record_t *));
#endif

    axis_sanitizer_memory_record_t *info = axis_ptr_listnode_get(iter.node);

    (void)fprintf(stderr, "\t#%zu %p(%zu bytes) in %s %s:%d\n", idx, info->addr,
                  info->size, info->func_name, info->file_name, info->lineno);

    idx++;

#if defined(axis_USE_ASAN)
    __asan_poison_memory_region((((axis_ptr_listnode_t *)(iter.node))->ptr),
                                sizeof(axis_sanitizer_memory_record_t *));
#endif
  }

#if defined(axis_USE_ASAN)
  __asan_poison_memory_region(&g_memory_records.records.front,
                              sizeof(axis_listnode_t *));
  __asan_poison_memory_region(&g_memory_records.records.back,
                              sizeof(axis_listnode_t *));
#endif

  size_t total_size = g_memory_records.total_size;

  rc = axis_mutex_unlock(g_memory_records.lock);
  axis_ASSERT(!rc, "Failed to unlock.");

  if (total_size) {
    (void)fprintf(stderr, "Memory leak with %zu bytes.\n", total_size);

#if defined(axis_USE_ASAN)
    __lsan_enable();
#endif

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(EXIT_FAILURE);
  }

#if defined(axis_USE_ASAN)
  __lsan_enable();
#endif

#else
  (void)fprintf(stderr, "The memory check is disabled.");
#endif
}

void *axis_sanitizer_memory_malloc(size_t size, const char *file_name,
                                  uint32_t lineno, const char *func_name) {
  void *self = malloc(size);
  axis_ASSERT(self, "Failed to allocate memory.");
  if (!self) {
    return NULL;
  }

  if (!g_memory_records_enabled) {
    goto done;
  }

  axis_sanitizer_memory_record_t *record = axis_sanitizer_memory_record_create(
      self, size, file_name, lineno, func_name);
  axis_ASSERT(record, "Should not happen.");
  if (!record) {
    free(self);
    return NULL;
  }

  axis_sanitizer_memory_record_add(&g_memory_records, record);

done:
  return self;
}

void *axis_sanitizer_memory_calloc(size_t cnt, size_t size,
                                  const char *file_name, uint32_t lineno,
                                  const char *func_name) {
  void *self = axis_calloc(cnt, size);
  axis_ASSERT(self, "Failed to allocate memory.");
  if (!self) {
    return NULL;
  }

  // If memory recording is not enabled, return the allocated memory.
  if (!g_memory_records_enabled) {
    goto done;
  }

  size_t total_size = cnt * size;

  // Create a memory record.
  axis_sanitizer_memory_record_t *record = axis_sanitizer_memory_record_create(
      self, total_size, file_name, lineno, func_name);
  axis_ASSERT(record, "Should not happen.");
  if (!record) {
    free(self);
    return NULL;
  }

  // Add the record to the global memory records.
  axis_sanitizer_memory_record_add(&g_memory_records, record);

done:
  return self;
}

void axis_sanitizer_memory_free(void *addr) {
  axis_ASSERT(addr, "Invalid argument.");

  if (!g_memory_records_enabled) {
    goto done;
  }

  axis_sanitizer_memory_record_del(&g_memory_records, addr);

done:
  free(addr);
}

void *axis_sanitizer_memory_realloc(void *addr, size_t size,
                                   const char *file_name, uint32_t lineno,
                                   const char *func_name) {
  // The address can be NULL, if it is NULL, a new block is allocated. The
  // return value maybe NULL if size is 0.
  void *self = axis_realloc(addr, size);

  if (!g_memory_records_enabled) {
    goto done;
  }

  if (self && self != addr) {
    // We only record a new memory.
    axis_sanitizer_memory_record_t *record = axis_sanitizer_memory_record_create(
        self, size, file_name, lineno, func_name);
    if (!record) {
      free(self);
      if (addr != NULL) {
        axis_sanitizer_memory_record_del(&g_memory_records, addr);
      }
      return NULL;
    }

    if (addr != NULL) {
      axis_sanitizer_memory_record_del(&g_memory_records, addr);
    }
    axis_sanitizer_memory_record_add(&g_memory_records, record);
  }

  if (addr && size == 0) {
    // If size is 0, the memory block pointed by address will be deallocated.
    axis_sanitizer_memory_record_del(&g_memory_records, addr);
  }

done:
  return self;
}

char *axis_sanitizer_memory_strdup(const char *str, const char *file_name,
                                  uint32_t lineno, const char *func_name) {
  axis_ASSERT(str, "Invalid argument.");

  char *self = axis_strdup(str);
  axis_ASSERT(self, "Failed to allocate memory.");
  if (!self) {
    return NULL;
  }

  if (!g_memory_records_enabled) {
    goto done;
  }

  axis_sanitizer_memory_record_t *record = axis_sanitizer_memory_record_create(
      self, strlen(self), file_name, lineno, func_name);
  if (!record) {
    free(self);
    return NULL;
  }

  axis_sanitizer_memory_record_add(&g_memory_records, record);

done:
  return self;
}

char *axis_sanitizer_memory_strndup(const char *str, size_t size,
                                   const char *file_name, uint32_t lineno,
                                   const char *func_name) {
  axis_ASSERT(str, "Invalid argument.");

  char *self = axis_strndup(str, size);
  axis_ASSERT(self, "Failed to allocate memory.");
  if (!self) {
    return NULL;
  }

  if (!g_memory_records_enabled) {
    goto done;
  }

  axis_sanitizer_memory_record_t *record = axis_sanitizer_memory_record_create(
      self, strlen(self), file_name, lineno, func_name);
  if (!record) {
    free(self);
    return NULL;
  }

  axis_sanitizer_memory_record_add(&g_memory_records, record);

done:
  return self;
}
