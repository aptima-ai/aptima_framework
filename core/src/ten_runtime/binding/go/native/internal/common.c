//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/binding/go/interface/aptima/common.h"

#include <stdlib.h>
#include <string.h>

#include "include_internal/ten_runtime/binding/go/internal/common.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"

ten_go_handle_array_t *ten_go_handle_array_create(size_t size) {
  ten_go_handle_array_t *self =
      (ten_go_handle_array_t *)TEN_MALLOC(sizeof(ten_go_handle_array_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  if (size == 0) {
    self->size = 0;
    self->array = NULL;
    return self;
  }

  self->size = size;
  self->array = (ten_go_handle_t *)TEN_MALLOC(sizeof(ten_go_handle_t) * size);
  TEN_ASSERT(self->array, "Failed to allocate memory.");

  return self;
}

void ten_go_handle_array_destroy(ten_go_handle_array_t *self) {
  TEN_ASSERT(self && self->array, "Should not happen.");

  if (self->array) {
    TEN_FREE(self->array);
  }
  TEN_FREE(self);
}

char *ten_go_str_dup(const char *str) { return strdup(str); }

void ten_go_bridge_destroy_c_part(ten_go_bridge_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (self->sp_ref_by_c != NULL) {
    ten_shared_ptr_destroy(self->sp_ref_by_c);
  }
}

void ten_go_bridge_destroy_go_part(ten_go_bridge_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (self->sp_ref_by_go != NULL) {
    ten_shared_ptr_destroy(self->sp_ref_by_go);
  }
}

void ten_go_error_init_with_errno(ten_go_error_t *self, ten_errno_t err_no) {
  TEN_ASSERT(self, "Should not happen.");

  self->err_no = err_no;
  self->err_msg_size = 0;
  self->err_msg = NULL;
}

void ten_go_error_from_error(ten_go_error_t *self, ten_error_t *err) {
  TEN_ASSERT(self && err, "Should not happen.");

  ten_go_error_set(self, ten_error_errno(err), ten_error_errmsg(err));
}

void ten_go_error_set_errno(ten_go_error_t *self, ten_errno_t err_no) {
  TEN_ASSERT(self, "Should not happen.");

  self->err_no = err_no;
}

void ten_go_error_set(ten_go_error_t *self, ten_errno_t err_no,
                      const char *msg) {
  TEN_ASSERT(self, "Should not happen.");

  self->err_no = err_no;
  if (msg == NULL || strlen(msg) == 0) {
    return;
  }

  uint8_t max_size = TEN_GO_STATUS_ERR_MSG_BUF_SIZE - 1;
  self->err_msg_size = strlen(msg) > max_size ? max_size : (uint8_t)strlen(msg);

  // This allocated memory space will be freed in the GO world.
  //
  // `C.free(unsafe.Pointer(error.err_msg))`
  self->err_msg = (char *)TEN_MALLOC(self->err_msg_size + 1);
  strncpy(self->err_msg, msg, self->err_msg_size);
  self->err_msg[self->err_msg_size] = '\0';
}

ten_go_error_t ten_go_copy_c_str_to_slice_and_free(const char *src,
                                                   void *dest) {
  TEN_ASSERT(src && dest, "Should not happen.");

  ten_go_error_t cgo_error;
  ten_go_error_init_with_errno(&cgo_error, TEN_ERRNO_OK);

  strcpy(dest, src);
  TEN_FREE(src);

  return cgo_error;
}
