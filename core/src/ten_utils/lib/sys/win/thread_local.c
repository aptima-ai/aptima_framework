//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/thread_local.h"

#include <Windows.h>

ten_thread_key_t ten_thread_key_create() {
  ten_thread_key_t key = TlsAlloc();

  if (key == TLS_OUT_OF_INDEXES) {
    return kInvalidTlsKey;
  }

  return key;
}

void ten_thread_key_destroy(ten_thread_key_t key) {
  if (key == TLS_OUT_OF_INDEXES || key == kInvalidTlsKey) {
    return;
  }

  TlsFree(key);
}

int ten_thread_set_key(ten_thread_key_t key, void *value) {
  if (key == TLS_OUT_OF_INDEXES || key == kInvalidTlsKey) {
    return -1;
  }

  return TlsSetValue(key, value) ? 0 : -1;
}

void *ten_thread_get_key(ten_thread_key_t key) {
  if (key == TLS_OUT_OF_INDEXES || key == kInvalidTlsKey) {
    return NULL;
  }

  return TlsGetValue(key);
}
