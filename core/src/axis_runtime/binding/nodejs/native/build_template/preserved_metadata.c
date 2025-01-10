//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/common/preserved_metadata.h"

static char metadata[] = "version=0.1.0";

void axis_preserved_metadata(void) {
  ((char volatile *)metadata)[0] = metadata[0];
}
