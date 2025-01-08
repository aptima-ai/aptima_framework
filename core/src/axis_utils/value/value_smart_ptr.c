//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/value/value_smart_ptr.h"

#include <stdlib.h>

#include "axis_utils/lib/error.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_kv.h"

bool axis_value_construct_for_smart_ptr(axis_value_t *self,
                                       axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_PTR, "Invalid argument.");
  axis_ASSERT(self->content.ptr, "Invalid argument.");

  return true;
}

bool axis_value_copy_for_smart_ptr(axis_value_t *dest, axis_value_t *src,
                                  axis_UNUSED axis_error_t *err) {
  axis_ASSERT(dest, "Invalid argument.");
  axis_ASSERT(src, "Invalid argument.");
  axis_ASSERT(src->type == axis_TYPE_PTR, "Invalid argument.");
  axis_ASSERT(src->content.ptr, "Invalid argument.");

  axis_LOGD("Copy c_value %p -> %p", src, dest);

  axis_value_reset_to_ptr(dest, axis_smart_ptr_clone(src->content.ptr),
                         src->construct, src->copy, src->destruct);

  return true;
}

bool axis_value_destruct_for_smart_ptr(axis_value_t *self,
                                      axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(self->type == axis_TYPE_PTR, "Invalid argument.");
  axis_ASSERT(self->content.ptr, "Invalid argument.");

  axis_LOGD("Delete c_value %p", self);

  axis_smart_ptr_destroy(self->content.ptr);
  self->content.ptr = NULL;

  return true;
}
