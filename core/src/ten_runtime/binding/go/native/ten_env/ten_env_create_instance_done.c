//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/extension/extension.h"
#include "include_internal/ten_runtime/binding/go/extension_group/extension_group.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/ten.h"

void ten_go_ten_env_on_create_instance_done(uintptr_t bridge_addr,
                                            bool is_extension,
                                            uintptr_t instance_bridge_addr,
                                            uintptr_t context_addr) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(instance_bridge_addr > 0, "Invalid argument.");
  TEN_ASSERT(context_addr > 0, "Invalid argument.");

  void *c_extension_or_extension_group = NULL;
  if (is_extension) {
    ten_go_extension_t *extension_bridge =
        ten_go_extension_reinterpret(instance_bridge_addr);
    TEN_ASSERT(ten_go_extension_check_integrity(extension_bridge),
               "Should not happen.");
    c_extension_or_extension_group =
        ten_go_extension_c_extension(extension_bridge);
  } else {
    ten_go_extension_group_t *extension_group_bridge =
        ten_go_extension_group_reinterpret(instance_bridge_addr);
    TEN_ASSERT(ten_go_extension_group_check_integrity(extension_group_bridge),
               "Should not happen.");
    c_extension_or_extension_group =
        ten_go_extension_group_c_extension_group(extension_group_bridge);
  }

  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(self, {});

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_env_on_create_instance_done(self->c_ten_env,
                                            c_extension_or_extension_group,
                                            (void *)context_addr, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);

  TEN_GO_TEN_IS_ALIVE_REGION_END(self);

ten_is_close:
  return;
}
