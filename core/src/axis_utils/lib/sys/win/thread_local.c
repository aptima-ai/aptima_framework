//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/thread_local.h"

#include <Windows.h>

axis_thread_key_t axis_thread_key_create() {
  axis_thread_key_t key = TlsAlloc();

  if (key == TLS_OUT_OF_INDEXES) {
    return kInvalidTlsKey;
  }

  return key;
}

void axis_thread_key_destroy(axis_thread_key_t key) {
  if (key == TLS_OUT_OF_INDEXES || key == kInvalidTlsKey) {
    return;
  }

  TlsFree(key);
}

int axis_thread_set_key(axis_thread_key_t key, void *value) {
  if (key == TLS_OUT_OF_INDEXES || key == kInvalidTlsKey) {
    return -1;
  }

  return TlsSetValue(key, value) ? 0 : -1;
}

void *axis_thread_get_key(axis_thread_key_t key) {
  if (key == TLS_OUT_OF_INDEXES || key == kInvalidTlsKey) {
    return NULL;
  }

  return TlsGetValue(key);
}
