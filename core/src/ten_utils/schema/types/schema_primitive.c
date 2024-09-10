//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_utils/schema/types/schema_primitive.h"

#include "include_internal/ten_utils/schema/schema.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/memory.h"

bool ten_schema_primitive_check_integrity(ten_schema_primitive_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  if (ten_signature_get(&self->signature) != TEN_SCHEMA_PRIMITIVE_SIGNATURE) {
    return false;
  }

  if (!ten_schema_check_integrity(&self->hdr)) {
    return false;
  }

  return true;
}

ten_schema_primitive_t *ten_schema_primitive_create(void) {
  ten_schema_primitive_t *self = TEN_MALLOC(sizeof(ten_schema_primitive_t));
  if (self == NULL) {
    TEN_ASSERT(0, "Failed to allocate memory.");
    return NULL;
  }

  ten_signature_set(&self->signature, TEN_SCHEMA_PRIMITIVE_SIGNATURE);
  ten_schema_init(&self->hdr);

  return self;
}

void ten_schema_primitive_destroy(ten_schema_primitive_t *self) {
  TEN_ASSERT(self && ten_schema_primitive_check_integrity(self),
             "Invalid argument.");

  ten_signature_set(&self->signature, 0);
  ten_schema_deinit(&self->hdr);

  TEN_FREE(self);
}
