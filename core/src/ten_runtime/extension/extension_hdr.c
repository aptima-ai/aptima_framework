//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension/extension_hdr.h"

#include "ten_utils/macro/check.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/memory.h"

ten_extensionhdr_t *ten_extensionhdr_create_for_extension(
    ten_extension_t *extension) {
  TEN_ASSERT(extension, "Invalid argument.");

  ten_extensionhdr_t *self = TEN_MALLOC(sizeof(ten_extensionhdr_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_signature_set(&self->signature,
                    (ten_signature_t)TEN_EXTENSIONHDR_SIGNATURE);
  ten_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  self->type = TEN_EXTENSION_TYPE_EXTENSION;
  self->u.extension = extension;

  return self;
}

ten_extensionhdr_t *ten_extensionhdr_create_for_extension_info(
    ten_smart_ptr_t *extension_info) {
  TEN_ASSERT(extension_info, "Invalid argument.");

  ten_extensionhdr_t *self = TEN_MALLOC(sizeof(ten_extensionhdr_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_signature_set(&self->signature,
                    (ten_signature_t)TEN_EXTENSIONHDR_SIGNATURE);
  ten_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  self->type = TEN_EXTENSION_TYPE_EXTENSION_INFO;
  self->u.extension_info = extension_info;

  return self;
}

void ten_extensionhdr_destroy(ten_extensionhdr_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  if (self->type == TEN_EXTENSION_TYPE_EXTENSION_INFO) {
    if (self->u.extension_info) {
      ten_shared_ptr_destroy(self->u.extension_info);
    }
  }

  ten_sanitizer_thread_check_deinit(&self->thread_check);
  ten_signature_set(&self->signature, 0);

  TEN_FREE(self);
}

bool ten_extensionhdr_check_integrity(ten_extensionhdr_t *self,
                                      bool check_thread) {
  TEN_ASSERT(self, "Invalid argument.");

  if (ten_signature_get(&self->signature) !=
      (ten_signature_t)TEN_EXTENSIONHDR_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !ten_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}
