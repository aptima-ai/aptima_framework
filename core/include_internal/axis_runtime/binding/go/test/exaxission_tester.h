//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/test/extension_tester.h"
#include "src/axis_runtime/binding/go/interface/ten/common.h"
#include "axis_utils/lib/signature.h"

#define axis_GO_EXTENSION_TESTER_SIGNATURE 0xF542240B13C47F46U

typedef struct axis_go_extension_tester_t {
  axis_signature_t signature;

  axis_go_bridge_t bridge;

  axis_extension_tester_t *c_extension_tester;
} axis_go_extension_tester_t;

axis_RUNTIME_PRIVATE_API bool axis_go_extension_tester_check_integrity(
    axis_go_extension_tester_t *self);

axis_RUNTIME_PRIVATE_API axis_go_extension_tester_t *
axis_go_extension_tester_reinterpret(uintptr_t bridge_addr);

axis_RUNTIME_PRIVATE_API axis_go_handle_t
axis_go_extension_tester_go_handle(axis_go_extension_tester_t *self);

axis_RUNTIME_API axis_go_extension_tester_t *
axis_go_extension_tester_create_internal(axis_go_handle_t go_extension_tester);

axis_RUNTIME_PRIVATE_API axis_extension_tester_t *
axis_go_extension_tester_c_extension_tester(axis_go_extension_tester_t *self);

axis_RUNTIME_PRIVATE_API void axis_go_extension_tester_set_go_handle(
    axis_go_extension_tester_t *self, axis_go_handle_t go_handle);
