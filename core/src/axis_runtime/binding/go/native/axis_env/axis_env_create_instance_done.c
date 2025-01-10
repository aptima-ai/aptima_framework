//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/binding/go/extension/extension.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "axis_runtime/binding/go/interface/aptima/axis_env.h"
#include "axis_runtime/extension/extension.h"
#include "axis_runtime/aptima.h"
#include "axis_utils/macro/check.h"

void axis_go_axis_env_on_create_instance_done(uintptr_t bridge_addr,
                                            uintptr_t instance_bridge_addr,
                                            uintptr_t context_addr) {
  axis_go_axis_env_t *self = axis_go_axis_env_reinterpret(bridge_addr);
  axis_ASSERT(self && axis_go_axis_env_check_integrity(self), "Invalid argument.");
  axis_ASSERT(instance_bridge_addr, "Invalid argument.");
  axis_ASSERT(context_addr, "Invalid argument.");

  axis_go_extension_t *extension_bridge =
      axis_go_extension_reinterpret(instance_bridge_addr);
  axis_ASSERT(axis_go_extension_check_integrity(extension_bridge),
             "Should not happen.");
  axis_extension_t *c_extension_or_extension_group =
      axis_go_extension_c_extension(extension_bridge);

  axis_GO_axis_ENV_IS_ALIVE_REGION_BEGIN(self, {});

  axis_error_t err;
  axis_error_init(&err);

  bool rc = axis_env_on_create_instance_done(self->c_axis_env,
                                            c_extension_or_extension_group,
                                            (void *)context_addr, &err);
  axis_ASSERT(rc, "Should not happen.");

  axis_error_deinit(&err);

  axis_GO_axis_ENV_IS_ALIVE_REGION_END(self);

axis_is_close:
  return;
}
