//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/common.h"

#include "include_internal/axis_runtime/binding/common.h"
#include "axis_utils/macro/check.h"

void axis_binding_handle_set_me_in_target_lang(axis_binding_handle_t *self,
                                              void *me_in_target_lang) {
  axis_ASSERT(self, "Invalid argument.");
  self->me_in_target_lang = me_in_target_lang;
}

void *axis_binding_handle_get_me_in_target_lang(axis_binding_handle_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  return self->me_in_target_lang;
}
