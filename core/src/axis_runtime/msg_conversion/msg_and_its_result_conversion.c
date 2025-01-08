//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg_conversion/msg_and_its_result_conversion.h"

#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"

axis_msg_and_its_result_conversion_t *axis_msg_and_its_result_conversion_create(
    axis_shared_ptr_t *msg, axis_msg_conversion_t *result_conversion) {
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");

  axis_msg_and_its_result_conversion_t *self =
      (axis_msg_and_its_result_conversion_t *)axis_MALLOC(
          sizeof(axis_msg_and_its_result_conversion_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  self->msg = axis_shared_ptr_clone(msg);

  self->result_conversion = result_conversion;

  return self;
}

void axis_msg_and_its_result_conversion_destroy(
    axis_msg_and_its_result_conversion_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (self->msg) {
    axis_shared_ptr_destroy(self->msg);
    self->msg = NULL;
  }

  axis_FREE(self);
}
