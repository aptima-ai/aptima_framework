//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <stdint.h>
#include <stdlib.h>

#include "include_internal/ten_runtime/binding/go/extension/extension.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env.h"
#include "include_internal/ten_runtime/binding/go/ten_env/ten_env_internal.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"
#include "ten_utils/lib/alloc.h"

typedef struct ten_env_notify_on_create_extensions_done_info_t {
  ten_list_t result;
} ten_env_notify_on_create_extensions_done_info_t;

static ten_env_notify_on_create_extensions_done_info_t *
ten_env_notify_on_create_extensions_done_info_create(ten_list_t *result) {
  TEN_ASSERT(result, "Invalid argument.");

  ten_env_notify_on_create_extensions_done_info_t *info =
      TEN_MALLOC(sizeof(ten_env_notify_on_create_extensions_done_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  ten_list_init(&info->result);
  ten_list_swap(&info->result, result);

  return info;
}

static void ten_env_notify_on_create_extensions_done_info_destroy(
    ten_env_notify_on_create_extensions_done_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  ten_list_clear(&info->result);

  TEN_FREE(info);
}

static void ten_notify_on_create_extensions_done(ten_env_t *ten_env,
                                                 void *user_data) {
  TEN_ASSERT(user_data, "Invalid argument.");
  TEN_ASSERT(
      ten_env &&
          ten_env_check_integrity(
              ten_env,
              ten_env->attach_to != TEN_ENV_ATTACH_TO_ADDON ? true : false),
      "Should not happen.");

  ten_env_notify_on_create_extensions_done_info_t *info = user_data;

  ten_error_t err;
  ten_error_init(&err);

  bool rc = ten_env_on_create_extensions_done(ten_env, &info->result, &err);
  TEN_ASSERT(rc, "Should not happen.");

  ten_error_deinit(&err);

  ten_env_notify_on_create_extensions_done_info_destroy(info);
}

void ten_go_ten_env_on_create_extensions_done(
    uintptr_t bridge_addr, const void *extension_bridge_array, int size) {
  ten_go_ten_env_t *self = ten_go_ten_env_reinterpret(bridge_addr);
  TEN_ASSERT(self && ten_go_ten_env_check_integrity(self),
             "Should not happen.");

  ten_list_t result = TEN_LIST_INIT_VAL;

  if (size == 0) {
    goto done;
  }

  uintptr_t *extensions = (uintptr_t *)extension_bridge_array;
  for (int i = 0; i < size; i++) {
    ten_go_extension_t *extension_bridge =
        ten_go_extension_reinterpret(extensions[i]);
    ten_list_push_ptr_back(
        &result, ten_go_extension_c_extension(extension_bridge), NULL);
  }

done:
  TEN_GO_TEN_IS_ALIVE_REGION_BEGIN(self, {});

  ten_error_t err;
  ten_error_init(&err);

  ten_env_notify_on_create_extensions_done_info_t
      *on_create_extensions_done_info =
          ten_env_notify_on_create_extensions_done_info_create(&result);

  if (!ten_env_proxy_notify(self->c_ten_env_proxy,
                            ten_notify_on_create_extensions_done,
                            on_create_extensions_done_info, false, &err)) {
    TEN_LOGD("TEN/GO failed to on_create_extensions_done.");

    ten_env_notify_on_create_extensions_done_info_destroy(
        on_create_extensions_done_info);
  }

  ten_error_deinit(&err);
  TEN_GO_TEN_IS_ALIVE_REGION_END(self);
ten_is_close:
  return;
}
