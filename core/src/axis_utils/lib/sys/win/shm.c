//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/shm.h"

#include <Windows.h>
#include <stdlib.h>

#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/thread_once.h"

/**
 * Copy from ddk (so sad..)
 * Conside make it a "common library"
 */
FORCEINLINE
VOID InitializeListHead(IN PLIST_ENTRY ListHead) {
  ListHead->Flink = ListHead->Blink = ListHead;
}

FORCEINLINE
BOOLEAN
RemoveEntryList(IN PLIST_ENTRY Entry) {
  PLIST_ENTRY Blink;
  PLIST_ENTRY Flink;

  Flink = Entry->Flink;
  Blink = Entry->Blink;
  Blink->Flink = Flink;
  Flink->Blink = Blink;
  return (BOOLEAN)(Flink == Blink);
}

FORCEINLINE
VOID InsertHeadList(IN PLIST_ENTRY ListHead, IN PLIST_ENTRY Entry) {
  PLIST_ENTRY Flink;

  Flink = ListHead->Flink;
  Entry->Flink = Flink;
  Entry->Blink = ListHead;
  Flink->Blink = Entry;
  ListHead->Flink = Entry;
}

FORCEINLINE
VOID InsertTailList(IN PLIST_ENTRY ListHead, IN PLIST_ENTRY Entry) {
  PLIST_ENTRY Blink;

  Blink = ListHead->Blink;
  Entry->Flink = ListHead;
  Entry->Blink = Blink;
  Blink->Flink = Entry;
  ListHead->Blink = Entry;
}

/**
 * Copy done
 */

typedef struct axis_shm_map_t {
  void *address;
  HANDLE file;
  LIST_ENTRY entry;
} axis_shm_map_t;

static CRITICAL_SECTION lock;
static LIST_ENTRY shm_map;
static axis_thread_once_t shm_map_init = axis_THREAD_ONCE_INIT;

static void __init_shm_map(void) {
  InitializeCriticalSection(&lock);
  InitializeListHead(&shm_map);
}

void *axis_shm_map(const char *name, size_t size) {
  HANDLE map_file = NULL;
  void *address = NULL;
  axis_shm_map_t *entry = NULL;

  if (!name || !*name || !size) {
    return NULL;
  }

  axis_thread_once(&shm_map_init, __init_shm_map);

  map_file = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                size + sizeof(axis_atomic_t), name);
  if (map_file == NULL) {
    goto error;
  }

  BOOL newly_created = (GetLastError() != ERROR_ALREADY_EXISTS);

  address = (void *)MapViewOfFile(map_file, FILE_MAP_ALL_ACCESS, 0, 0,
                                  size + sizeof(axis_atomic_t));
  if (!address) {
    goto error;
  }

  if (newly_created) {
    memset(address, 0, size + sizeof(axis_atomic_t));
    axis_atomic_store((axis_atomic_t *)address, size);
  }

  address = (char *)address + sizeof(axis_atomic_t);

  entry = (axis_shm_map_t *)malloc(sizeof(*entry));
  memset(entry, 0, sizeof(*entry));
  entry->address = address;
  entry->file = map_file;
  EnterCriticalSection(&lock);
  InsertHeadList(&shm_map, &entry->entry);
  LeaveCriticalSection(&lock);

  return address;

error:
  if (map_file) {
    CloseHandle(map_file);
  }

  return address;
}

void axis_shm_unmap(void *addr) {
  void *begin = (char *)addr - sizeof(axis_atomic_t);
  size_t size = axis_shm_get_size(addr);
  axis_shm_map_t *entry = NULL;
  LIST_ENTRY *itor = NULL;

  if (!addr || !size) {
    return;
  }

  UnmapViewOfFile(begin);

  EnterCriticalSection(&lock);
  for (itor = shm_map.Flink; itor && itor != &shm_map; itor = itor->Flink) {
    axis_shm_map_t *tmp =
        (axis_shm_map_t *)CONTAINING_RECORD(itor, axis_shm_map_t, entry);
    if (tmp->address != addr) {
      continue;
    }

    entry = tmp;
    break;
  }

  if (entry) {
    RemoveEntryList(itor);
    CloseHandle(entry->file);
    free(entry);
  }

  LeaveCriticalSection(&lock);
}

size_t axis_shm_get_size(void *addr) {
  void *begin = (char *)addr - sizeof(axis_atomic_t);

  return addr ? axis_atomic_load((volatile axis_atomic_t *)begin) : 0;
}

void axis_shm_unlink(const char *name) {
  // Do nothing!
}