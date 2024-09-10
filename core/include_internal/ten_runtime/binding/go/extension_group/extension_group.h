//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/extension/extension.h"
#include "src/ten_runtime/binding/go/interface/ten/common.h"
#include "ten_utils/lib/signature.h"

#define TEN_GO_EXTENSION_GROUP_SIGNATURE 0xCD7EC4A88C000EC0U

typedef struct ten_go_extension_group_t {
  ten_signature_t signature;

  ten_go_bridge_t bridge;

  ten_extension_group_t *c_extension_group;
  const char *name;
} ten_go_extension_group_t;

TEN_RUNTIME_PRIVATE_API bool ten_go_extension_group_check_integrity(
    ten_go_extension_group_t *self);

TEN_RUNTIME_PRIVATE_API ten_go_extension_group_t *
ten_go_extension_group_reinterpret(uintptr_t extension_group_bridge);

TEN_RUNTIME_PRIVATE_API ten_go_handle_t
ten_go_extension_group_go_handle(ten_go_extension_group_t *self);

TEN_RUNTIME_PRIVATE_API ten_go_extension_group_t *
ten_go_extension_group_create_internal(ten_go_handle_t go_extension_group,
                                       const char *name);

TEN_RUNTIME_PRIVATE_API ten_extension_group_t *
ten_go_extension_group_c_extension_group(ten_go_extension_group_t *self);

TEN_RUNTIME_PRIVATE_API void ten_go_extension_group_set_go_handle(
    ten_go_extension_group_t *self, ten_go_handle_t go_handle);
