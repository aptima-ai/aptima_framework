//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/go/interface/ten/common.h"

#include <stdlib.h>
#include <string.h>

#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

axis_go_handle_array_t *axis_go_handle_array_create(size_t size) {
  axis_go_handle_array_t *self =
      (axis_go_handle_array_t *)axis_MALLOC(sizeof(axis_go_handle_array_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  if (size == 0) {
    self->size = 0;
    self->array = NULL;
    return self;
  }

  self->size = size;
  self->array = (axis_go_handle_t *)axis_MALLOC(sizeof(axis_go_handle_t) * size);
  axis_ASSERT(self->array, "Failed to allocate memory.");

  return self;
}

void axis_go_handle_array_destroy(axis_go_handle_array_t *self) {
  axis_ASSERT(self && self->array, "Should not happen.");

  if (self->array) {
    axis_FREE(self->array);
  }
  axis_FREE(self);
}

char *axis_go_str_dup(const char *str) { return strdup(str); }

void axis_go_bridge_destroy_c_part(axis_go_bridge_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (self->sp_ref_by_c != NULL) {
    axis_shared_ptr_destroy(self->sp_ref_by_c);
  }
}

void axis_go_bridge_destroy_go_part(axis_go_bridge_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (self->sp_ref_by_go != NULL) {
    axis_shared_ptr_destroy(self->sp_ref_by_go);
  }
}

void axis_go_error_init_with_errno(axis_go_error_t *self, axis_errno_t err_no) {
  axis_ASSERT(self, "Should not happen.");

  self->err_no = err_no;
  self->err_msg_size = 0;
  self->err_msg = NULL;
}

void axis_go_error_from_error(axis_go_error_t *self, axis_error_t *err) {
  axis_ASSERT(self && err, "Should not happen.");

  axis_go_error_set(self, axis_error_errno(err), axis_error_errmsg(err));
}

void axis_go_error_set_errno(axis_go_error_t *self, axis_errno_t err_no) {
  axis_ASSERT(self, "Should not happen.");

  self->err_no = err_no;
}

void axis_go_error_set(axis_go_error_t *self, axis_errno_t err_no,
                      const char *msg) {
  axis_ASSERT(self, "Should not happen.");

  self->err_no = err_no;
  if (msg == NULL || strlen(msg) == 0) {
    return;
  }

  uint8_t max_size = axis_GO_STATUS_ERR_MSG_BUF_SIZE - 1;
  self->err_msg_size = strlen(msg) > max_size ? max_size : (uint8_t)strlen(msg);

  // This allocated memory space will be freed in the GO world.
  //
  // `C.free(unsafe.Pointer(error.err_msg))`
  self->err_msg = (char *)axis_MALLOC(self->err_msg_size + 1);
  strncpy(self->err_msg, msg, self->err_msg_size);
  self->err_msg[self->err_msg_size] = '\0';
}

axis_go_error_t axis_go_copy_c_str_to_slice_and_free(const char *src,
                                                   void *dest) {
  axis_ASSERT(src && dest, "Should not happen.");

  axis_go_error_t cgo_error;
  axis_go_error_init_with_errno(&cgo_error, axis_ERRNO_OK);

  strcpy(dest, src);
  axis_FREE(src);

  return cgo_error;
}
