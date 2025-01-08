//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/signature.h"

#include "axis_utils/macro/check.h"

void axis_signature_set(axis_signature_t *signature, axis_signature_t value) {
  axis_ASSERT(signature, "Invalid argument.");
  *signature = value;
}

axis_signature_t axis_signature_get(const axis_signature_t *signature) {
  axis_ASSERT(signature, "Invalid argument.");
  return *signature;
}
