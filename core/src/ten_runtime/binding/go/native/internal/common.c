//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_runtime/binding/go/interface/ten/common.h"

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

void ten_go_status_init_with_errno(ten_go_status_t *self, ten_errno_t errno) {
  TEN_ASSERT(self, "Should not happen.");

  self->errno = errno;
  self->msg_size = 0;
  self->err_msg = NULL;
}

void ten_go_status_from_error(ten_go_status_t *self, ten_error_t *err) {
  TEN_ASSERT(self && err, "Should not happen.");

  ten_go_status_set(self, ten_error_errno(err), ten_error_errmsg(err));
}

void ten_go_status_set_errno(ten_go_status_t *self, ten_errno_t errno) {
  TEN_ASSERT(self, "Should not happen.");

  self->errno = errno;
}

void ten_go_status_set(ten_go_status_t *self, ten_errno_t errno,
                       const char *msg) {
  TEN_ASSERT(self, "Should not happen.");

  self->errno = errno;
  if (msg == NULL || strlen(msg) == 0) {
    return;
  }

  uint8_t max_size = TEN_GO_STATUS_ERR_MSG_BUF_SIZE - 1;
  self->msg_size = strlen(msg) > max_size ? max_size : (uint8_t)strlen(msg);

  // This allocated memory space will be freed in the GO world.
  //
  // `C.free(unsafe.Pointer(status.err_msg))`
  self->err_msg = (char *)TEN_MALLOC(self->msg_size + 1);
  strncpy(self->err_msg, msg, self->msg_size);
  self->err_msg[self->msg_size] = '\0';
}

ten_go_status_t ten_go_copy_c_str_to_slice_and_free(const char *src,
                                                    void *dest) {
  TEN_ASSERT(src && dest, "Should not happen.");

  ten_go_status_t status;
  ten_go_status_init_with_errno(&status, TEN_ERRNO_OK);

  strcpy(dest, src);
  TEN_FREE(src);

  return status;
}
