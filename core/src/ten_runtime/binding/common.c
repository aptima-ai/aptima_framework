//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_runtime/binding/common.h"

#include "include_internal/ten_runtime/binding/common.h"
#include "ten_utils/macro/check.h"

void ten_binding_handle_set_me_in_target_lang(ten_binding_handle_t *self,
                                              void *me_in_target_lang) {
  TEN_ASSERT(self, "Invalid argument.");
  self->me_in_target_lang = me_in_target_lang;
}

void *ten_binding_handle_get_me_in_target_lang(ten_binding_handle_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  return self->me_in_target_lang;
}
