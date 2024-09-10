//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/lib/signature.h"

#include "ten_utils/macro/check.h"

void ten_signature_set(ten_signature_t *signature, ten_signature_t value) {
  TEN_ASSERT(signature, "Invalid argument.");
  *signature = value;
}

ten_signature_t ten_signature_get(const ten_signature_t *signature) {
  TEN_ASSERT(signature, "Invalid argument.");
  return *signature;
}
