//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/sanitizer/memory_check.h"

#include "include_internal/ten_utils/lib/alloc.h"

#if defined(TEN_USE_ASAN)
#include <sanitizer/asan_interface.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "include_internal/ten_utils/sanitizer/memory_check.h"
#include "ten_utils/container/list.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

static ten_sanitizer_memory_records_t g_memory_records = {NULL,
                                                          TEN_LIST_INIT_VAL, 0};
static bool g_memory_records_enabled = false;

static void ten_sanitizer_memory_record_check_enabled(void) {
#if defined(TEN_ENABLE_MEMORY_CHECK)
  char *enable_memory_sanitizer = getenv("TEN_ENABLE_MEMORY_TRACKING");
  if (enable_memory_sanitizer && !strcmp(enable_memory_sanitizer, "true")) {
    g_memory_records_enabled = true;
  }
#else
  TEN_LOGI("The memory check is disabled.");
#endif
}

void ten_sanitizer_memory_record_init(void) {
  ten_sanitizer_memory_record_check_enabled();

#if defined(TEN_USE_ASAN)
  // Mark the beginning and end of the globally allocated memory queue as
  // poisoned, so that LSan will not consider the memory buffer obtained from
  // there as normal memory, but will instead consider it as leaked.
  __asan_poison_memory_region(&g_memory_records.records.front,
                              sizeof(ten_listnode_t *));
  __asan_poison_memory_region(&g_memory_records.records.back,
                              sizeof(ten_listnode_t *));
#endif

  g_memory_records.lock = ten_mutex_create();
}

void ten_sanitizer_memory_record_deinit(void) {
  ten_sanitizer_memory_record_dump();

  if (g_memory_records.lock) {
    ten_mutex_destroy(g_memory_records.lock);
  }
}

static ten_sanitizer_memory_record_t *ten_sanitizer_memory_record_create(
    void *addr, size_t size, const char *file_name, uint32_t lineno,
    const char *func_name) {
  ten_sanitizer_memory_record_t *self =
      ten_malloc(sizeof(ten_sanitizer_memory_record_t));
  TEN_ASSERT(self, "Failed to allocate memory.");
  if (!self) {
    return NULL;
  }

  self->addr = addr;
  self->size = size;

  ten_string_init_formatted(&self->func_name, "%s", func_name);

  ten_string_init_formatted(
      &self->file_name, "%.*s",
      strlen(file_name) - TEN_FILE_PATH_RELATIVE_PREFIX_LENGTH,
      file_name + TEN_FILE_PATH_RELATIVE_PREFIX_LENGTH);

  self->lineno = lineno;

  return self;
}

static void ten_sanitizer_memory_record_destroy(
    ten_sanitizer_memory_record_t *self) {
  ten_string_deinit(&self->file_name);
  ten_free(self);
}

static void ten_sanitizer_memory_record_add(
    ten_sanitizer_memory_records_t *self,
    ten_sanitizer_memory_record_t *record) {
  TEN_ASSERT(self && record, "Invalid argument.");

  TEN_UNUSED int rc = ten_mutex_lock(self->lock);
  TEN_ASSERT(!rc, "Failed to lock.");

#if defined(TEN_USE_ASAN)
  __asan_unpoison_memory_region(&self->records.front, sizeof(ten_listnode_t *));
  __asan_unpoison_memory_region(&self->records.back, sizeof(ten_listnode_t *));
#endif

  ten_list_push_ptr_back(
      &self->records, record,
      (ten_ptr_listnode_destroy_func_t)ten_sanitizer_memory_record_destroy);

#if defined(TEN_USE_ASAN)
  __asan_poison_memory_region(
      (((ten_ptr_listnode_t *)(self->records.back))->ptr),
      sizeof(ten_sanitizer_memory_record_t *));

  __asan_poison_memory_region(&self->records.front, sizeof(ten_listnode_t *));
  __asan_poison_memory_region(&self->records.back, sizeof(ten_listnode_t *));
#endif

  self->total_size += record->size;

  rc = ten_mutex_unlock(self->lock);
  TEN_ASSERT(!rc, "Failed to unlock.");
}

static void ten_sanitizer_memory_record_del(
    ten_sanitizer_memory_records_t *self, void *addr) {
  TEN_ASSERT(self && addr, "Invalid argument.");

  TEN_UNUSED int rc = ten_mutex_lock(self->lock);
  TEN_ASSERT(!rc, "Failed to lock.");

#if defined(TEN_USE_ASAN)
  __asan_unpoison_memory_region(&self->records.front, sizeof(ten_listnode_t *));
  __asan_unpoison_memory_region(&self->records.back, sizeof(ten_listnode_t *));
#endif

  ten_list_foreach (&self->records, iter) {
#if defined(TEN_USE_ASAN)
    __asan_unpoison_memory_region((((ten_ptr_listnode_t *)(iter.node))->ptr),
                                  sizeof(ten_sanitizer_memory_record_t *));
#endif

    ten_sanitizer_memory_record_t *record = ten_ptr_listnode_get(iter.node);

    if (record->addr == addr) {
      self->total_size -= record->size;
      ten_list_remove_node(&self->records, iter.node);
      break;
    }

#if defined(TEN_USE_ASAN)
    __asan_poison_memory_region((((ten_ptr_listnode_t *)(iter.node))->ptr),
                                sizeof(ten_sanitizer_memory_record_t *));
#endif
  }

#if defined(TEN_USE_ASAN)
  __asan_poison_memory_region(&self->records.front, sizeof(ten_listnode_t *));
  __asan_poison_memory_region(&self->records.back, sizeof(ten_listnode_t *));
#endif

  rc = ten_mutex_unlock(self->lock);
  TEN_ASSERT(!rc, "Failed to unlock.");
}

void ten_sanitizer_memory_record_dump(void) {
#if defined(TEN_ENABLE_MEMORY_CHECK)
  TEN_UNUSED int rc = ten_mutex_lock(g_memory_records.lock);
  TEN_ASSERT(!rc, "Failed to lock.");

  if (g_memory_records.total_size) {
    TEN_LOGD("Memory allocation summary(%zu bytes):",
             g_memory_records.total_size);
  }

#if defined(TEN_USE_ASAN)
  __asan_unpoison_memory_region(&g_memory_records.records.front,
                                sizeof(ten_listnode_t *));
  __asan_unpoison_memory_region(&g_memory_records.records.back,
                                sizeof(ten_listnode_t *));
#endif

  size_t idx = 0;
  ten_list_foreach (&g_memory_records.records, iter) {
#if defined(TEN_USE_ASAN)
    __asan_unpoison_memory_region((((ten_ptr_listnode_t *)(iter.node))->ptr),
                                  sizeof(ten_sanitizer_memory_record_t *));
#endif

    ten_sanitizer_memory_record_t *info = ten_ptr_listnode_get(iter.node);

    TEN_LOGE("\t#%ld %p(%ld bytes) in %s %s:%d", idx, info->addr, info->size,
             ten_string_get_raw_str(&info->func_name),
             ten_string_get_raw_str(&info->file_name), info->lineno);

    idx++;

#if defined(TEN_USE_ASAN)
    __asan_poison_memory_region((((ten_ptr_listnode_t *)(iter.node))->ptr),
                                sizeof(ten_sanitizer_memory_record_t *));
#endif
  }

#if defined(TEN_USE_ASAN)
  __asan_poison_memory_region(&g_memory_records.records.front,
                              sizeof(ten_listnode_t *));
  __asan_poison_memory_region(&g_memory_records.records.back,
                              sizeof(ten_listnode_t *));
#endif

  size_t total_size = g_memory_records.total_size;

  rc = ten_mutex_unlock(g_memory_records.lock);
  TEN_ASSERT(!rc, "Failed to unlock.");

  if (total_size) {
    TEN_LOGE("Memory leak with %zu bytes.", total_size);
    exit(EXIT_FAILURE);
  }
#else
  TEN_LOGI("The memory check is disabled.");
#endif
}

void *ten_sanitizer_memory_malloc(size_t size, const char *file_name,
                                  uint32_t lineno, const char *func_name) {
  void *self = ten_malloc(size);
  TEN_ASSERT(self, "Failed to allocate memory.");
  if (!self) {
    return NULL;
  }

  if (!g_memory_records_enabled) {
    goto done;
  }

  ten_sanitizer_memory_record_t *record = ten_sanitizer_memory_record_create(
      self, size, file_name, lineno, func_name);
  TEN_ASSERT(record, "Should not happen.");
  if (!record) {
    ten_free(self);
    return NULL;
  }

  ten_sanitizer_memory_record_add(&g_memory_records, record);

done:
  return self;
}

void *ten_sanitizer_memory_calloc(size_t cnt, size_t size,
                                  const char *file_name, uint32_t lineno,
                                  const char *func_name) {
  void *self = ten_calloc(cnt, size);
  TEN_ASSERT(self, "Failed to allocate memory.");
  if (!self) {
    return NULL;
  }

  // If memory recording is not enabled, return the allocated memory.
  if (!g_memory_records_enabled) {
    goto done;
  }

  size_t total_size = cnt * size;

  // Create a memory record.
  ten_sanitizer_memory_record_t *record = ten_sanitizer_memory_record_create(
      self, total_size, file_name, lineno, func_name);
  TEN_ASSERT(record, "Should not happen.");
  if (!record) {
    ten_free(self);
    return NULL;
  }

  // Add the record to the global memory records.
  ten_sanitizer_memory_record_add(&g_memory_records, record);

done:
  return self;
}

void ten_sanitizer_memory_free(void *addr) {
  TEN_ASSERT(addr, "Invalid argument.");

  if (!g_memory_records_enabled) {
    goto done;
  }

  ten_sanitizer_memory_record_del(&g_memory_records, addr);

done:
  ten_free(addr);
}

void *ten_sanitizer_memory_realloc(void *addr, size_t size,
                                   const char *file_name, uint32_t lineno,
                                   const char *func_name) {
  // The address can be NULL, if it is NULL, a new block is allocated. The
  // return value maybe NULL if size is 0.
  void *self = ten_realloc(addr, size);

  if (!g_memory_records_enabled) {
    goto done;
  }

  if (self && self != addr) {
    // We only record a new memory.
    ten_sanitizer_memory_record_t *record = ten_sanitizer_memory_record_create(
        self, size, file_name, lineno, func_name);
    if (!record) {
      ten_free(self);
      ten_sanitizer_memory_record_del(&g_memory_records, addr);
      return NULL;
    }

    ten_sanitizer_memory_record_add(&g_memory_records, record);
  }

  if (addr && size == 0) {
    // If size is 0, the memory block pointed by address will be deallocated.
    ten_sanitizer_memory_record_del(&g_memory_records, addr);
  }

done:
  return self;
}

char *ten_sanitizer_memory_strdup(const char *str, const char *file_name,
                                  uint32_t lineno, const char *func_name) {
  TEN_ASSERT(str, "Invalid argument.");

  char *self = ten_strdup(str);
  TEN_ASSERT(self, "Failed to allocate memory.");
  if (!self) {
    return NULL;
  }

  if (!g_memory_records_enabled) {
    goto done;
  }

  ten_sanitizer_memory_record_t *record = ten_sanitizer_memory_record_create(
      self, strlen(self), file_name, lineno, func_name);
  if (!record) {
    ten_free(self);
    return NULL;
  }

  ten_sanitizer_memory_record_add(&g_memory_records, record);

done:
  return self;
}

char *ten_sanitizer_memory_strndup(const char *str, size_t size,
                                   const char *file_name, uint32_t lineno,
                                   const char *func_name) {
  TEN_ASSERT(str, "Invalid argument.");

  char *self = ten_strndup(str, size);
  TEN_ASSERT(self, "Failed to allocate memory.");
  if (!self) {
    return NULL;
  }

  if (!g_memory_records_enabled) {
    goto done;
  }

  ten_sanitizer_memory_record_t *record = ten_sanitizer_memory_record_create(
      self, strlen(self), file_name, lineno, func_name);
  if (!record) {
    ten_free(self);
    return NULL;
  }

  ten_sanitizer_memory_record_add(&g_memory_records, record);

done:
  return self;
}
