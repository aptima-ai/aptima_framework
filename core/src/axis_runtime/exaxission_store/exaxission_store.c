//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension_store/extension_store.h"

#include <stddef.h>
#include <stdlib.h>

#include "include_internal/axis_runtime/extension/extension.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/field.h"
#include "axis_utils/sanitizer/thread_check.h"

static bool axis_extension_store_check_integrity(axis_extension_store_t *self,
                                                bool check_thread) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_EXTENSION_STORE_SIGNATURE) {
    return false;
  }

  if (check_thread &&
      !axis_sanitizer_thread_check_do_check(&self->thread_check)) {
    return false;
  }

  return true;
}

static void axis_extension_store_init(axis_extension_store_t *self,
                                     ptrdiff_t hh_offset) {
  axis_ASSERT(self, "Invalid argument.");

  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_EXTENSION_STORE_SIGNATURE);

  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  axis_hashtable_init(&self->hash_table, hh_offset);
}

axis_extension_store_t *axis_extension_store_create(ptrdiff_t offset) {
  axis_extension_store_t *self =
      (axis_extension_store_t *)axis_MALLOC(sizeof(axis_extension_store_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_extension_store_init(self, offset);
  return self;
}

static void axis_extension_store_deinit(axis_extension_store_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: extension_store will be destroyed _after_ the extension
  // thread is joined. So we can not check the thread safety here.
  axis_ASSERT(axis_extension_store_check_integrity(self, false),
             "Invalid use of extension_store %p.", self);

  axis_hashtable_deinit(&self->hash_table);
  axis_sanitizer_thread_check_deinit(&self->thread_check);

  axis_signature_set(&self->signature, 0);
}

void axis_extension_store_destroy(axis_extension_store_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: extension_store will be destroyed _after_ the extension
  // thread is joined. So we can not check the thread safety here.
  axis_ASSERT(axis_extension_store_check_integrity(self, false),
             "Invalid use of extension_store %p.", self);

  axis_extension_store_deinit(self);
  axis_FREE(self);
}

bool axis_extension_store_add_extension(axis_extension_store_t *self,
                                       axis_extension_t *extension) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_store_check_integrity(self, true),
             "Invalid use of extension_store %p.", self);

  axis_ASSERT(extension, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  bool result = true;
  axis_hashhandle_t *found = axis_hashtable_find_string(
      &self->hash_table, axis_string_get_raw_str(&extension->name));
  if (found) {
    axis_LOGE("Failed to have extension with name: %s",
             axis_extension_get_name(extension, true));
    result = false;
    goto done;
  }

  axis_hashtable_add_string(&self->hash_table, &extension->hh_in_extension_store,
                           axis_string_get_raw_str(&extension->name), NULL);

done:
  return result;
}

void axis_extension_store_del_extension(axis_extension_store_t *self,
                                       axis_extension_t *extension) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_store_check_integrity(self, true),
             "Invalid use of extension_store %p.", self);

  axis_ASSERT(extension, "Invalid argument.");
  axis_ASSERT(axis_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  axis_hashtable_del(&self->hash_table, &extension->hh_in_extension_store);
}

axis_extension_t *axis_extension_store_find_extension(axis_extension_store_t *self,
                                                    const char *extension_name,
                                                    bool check_thread) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_extension_store_check_integrity(self, check_thread),
             "Invalid use of extension_store %p.", self);

  axis_ASSERT(extension_name, "Should not happen.");

  axis_extension_t *extension = NULL;

  axis_hashhandle_t *hh =
      axis_hashtable_find_string(&self->hash_table, extension_name);
  if (hh) {
    extension =
        CONTAINER_OF_FROM_FIELD(hh, axis_extension_t, hh_in_extension_store);

    return extension;
  }

  return extension;
}
