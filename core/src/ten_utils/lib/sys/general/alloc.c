//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/alloc.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "include_internal/ten_utils/backtrace/backtrace.h"
#include "include_internal/ten_utils/lib/alloc.h"

void *ten_malloc_without_backtrace(size_t size) {
  if (!size) {
    assert(0 && "malloc of size 0 is implementation defined behavior.");
    return NULL;
  }

  void *result = malloc(size);
  if (!result) {
    assert(0 && "Failed to allocate memory.");
  }

  return result;
}

void *ten_malloc(size_t size) {
  void *result = ten_malloc_without_backtrace(size);
  if (!result) {
    ten_backtrace_dump_global(0);
    exit(EXIT_FAILURE);
  }

  return result;
}

void *ten_calloc_without_backtrace(size_t cnt, size_t size) {
  if (!cnt || !size) {
    assert(0 && "calloc of size 0 is implementation defined behavior.");
    return NULL;
  }

  void *result = calloc(cnt, size);
  if (!result) {
    assert(0 && "Failed to allocate memory.");
  }

  return result;
}

void *ten_calloc(size_t cnt, size_t size) {
  void *result = ten_calloc_without_backtrace(cnt, size);
  if (!result) {
    ten_backtrace_dump_global(0);
    exit(EXIT_FAILURE);
  }

  return result;
}

void *ten_realloc_without_backtrace(void *p, size_t size) {
  if (!size) {
    assert(0 && "realloc of size 0 is implementation defined behavior.");
    return NULL;
  }

  void *result = realloc(p, size);
  if (!result) {
    assert(0 && "Failed to allocate memory.");
  }

  return result;
}

void *ten_realloc(void *p, size_t size) {
  void *result = ten_realloc_without_backtrace(p, size);
  if (!result) {
    ten_backtrace_dump_global(0);
    exit(EXIT_FAILURE);
  }

  return result;
}

void ten_free_without_backtrace(void *p) {
  assert(p);
  free(p);
}

void ten_free(void *p) {
  assert(p);
  ten_free_without_backtrace(p);
}

char *ten_strdup_without_backtrace(const char *str) {
  if (!str) {
    assert(0 && "Invalid argument.");
    return NULL;
  }

  char *result = strdup(str);
  if (!result) {
    assert(0 && "Failed to allocate memory.");
  }

  return result;
}

char *ten_strdup(const char *str) {
  char *result = ten_strdup_without_backtrace(str);
  if (!result) {
    ten_backtrace_dump_global(0);
    exit(EXIT_FAILURE);
  }

  return result;
}

char *ten_strndup_without_backtrace(const char *str, size_t size) {
  if (!str) {
    assert(0 && "Invalid argument.");
    return NULL;
  }

  char *result = NULL;

#if defined(OS_WINDOWS)
  result = (char *)malloc(size + 1);
#else
  result = strndup(str, size);
#endif

  if (!result) {
    assert(0 && "Failed to allocate memory.");
    return NULL;
  }

#if defined(OS_WINDOWS)
  strncpy(result, str, size);
  result[size] = '\0';
#endif

  return result;
}

char *ten_strndup(const char *str, size_t size) {
  char *result = ten_strndup_without_backtrace(str, size);
  if (!result) {
    ten_backtrace_dump_global(0);
    exit(EXIT_FAILURE);
  }

  return result;
}
