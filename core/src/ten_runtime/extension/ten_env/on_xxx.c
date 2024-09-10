//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/extension/on_xxx.h"

#include "include_internal/ten_runtime/common/loc.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension/path_timer.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/msg_interface/common.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

static ten_extension_on_init_done_t *ten_extension_on_init_done_create(
    ten_extension_t *extension) {
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  ten_extension_on_init_done_t *on_init_done =
      TEN_MALLOC(sizeof(ten_extension_on_init_done_t));
  TEN_ASSERT(on_init_done, "Failed to allocate memory.");

  on_init_done->extension = extension;

  return on_init_done;
}

void ten_extension_on_init_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  TEN_LOGD("[%s] on_init() done.", ten_extension_get_name(extension));

  ten_extension_thread_t *extension_thread = extension->extension_thread;
  TEN_ASSERT(extension_thread &&
                 ten_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  ten_extension_on_init_done_t *on_init_done =
      ten_extension_on_init_done_create(extension);

  ten_runloop_post_task_tail(ten_extension_get_attached_runloop(extension),
                             ten_extension_thread_on_extension_on_init_done,
                             extension_thread, on_init_done);
}

void ten_extension_on_start_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  TEN_LOGI("[%s] on_start() done.", ten_extension_get_name(extension));

  ten_extension_thread_t *extension_thread = extension->extension_thread;
  TEN_ASSERT(extension_thread &&
                 ten_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  // Notify that the extension is started completely.
  ten_extension_on_start_stop_deinit_done_t *on_start_done =
      ten_extension_on_start_stop_deinit_done_create(extension);

  ten_runloop_post_task_tail(ten_extension_get_attached_runloop(extension),
                             ten_extension_thread_on_extension_on_start_done,
                             extension_thread, on_start_done);
}

void ten_extension_on_stop_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  TEN_LOGI("[%s] on_stop() done.", ten_extension_get_name(extension));

  // When we reach here, it means the extension has been stopped completely.
  // Notify the extension thread about this fact.

  ten_extension_on_start_stop_deinit_done_t *on_stop_done =
      ten_extension_on_start_stop_deinit_done_create(extension);

  // Use the runloop task mechanism to ensure the operations afterwards will be
  // executed in the extension thread.
  ten_runloop_post_task_tail(ten_extension_get_attached_runloop(extension),
                             ten_extension_thread_on_extension_on_stop_done,
                             extension->extension_thread, on_stop_done);
}

void ten_extension_on_deinit_done(ten_env_t *self) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);

  ten_extension_t *extension = ten_env_get_attached_extension(self);
  TEN_ASSERT(extension, "Invalid argument.");
  TEN_ASSERT(ten_extension_check_integrity(extension, true),
             "Invalid use of extension %p.", extension);

  if (!ten_list_is_empty(&self->ten_proxy_list)) {
    // There is still the presence of ten_env_proxy, so the closing process
    // cannot continue.
    TEN_LOGI(
        "[%s] Failed to on_deinit_done() because of existed ten_env_proxy.",
        ten_extension_get_name(extension));
    return;
  }

  TEN_ASSERT(extension->state >= TEN_EXTENSION_STATE_DEINITING,
             "Should not happen.");

  if (extension->state == TEN_EXTENSION_STATE_DEINITED) {
    return;
  }

  ten_extension_set_state(extension, TEN_EXTENSION_STATE_DEINITED);

  TEN_LOGD("[%s] on_deinit() done.", ten_extension_get_name(extension));

  ten_extension_on_start_stop_deinit_done_t *on_deinit_done =
      ten_extension_on_start_stop_deinit_done_create(extension);

  ten_runloop_post_task_tail(ten_extension_get_attached_runloop(extension),
                             ten_extension_thread_on_extension_on_deinit_done,
                             extension->extension_thread, on_deinit_done);
}

void ten_extension_on_init_done_destroy(ten_extension_on_init_done_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  TEN_FREE(self);
}

ten_extension_on_start_stop_deinit_done_t *
ten_extension_on_start_stop_deinit_done_create(ten_extension_t *extension) {
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  ten_extension_on_start_stop_deinit_done_t *self =
      TEN_MALLOC(sizeof(ten_extension_on_start_stop_deinit_done_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->extension = extension;

  return self;
}

void ten_extension_on_start_stop_deinit_done_destroy(
    ten_extension_on_start_stop_deinit_done_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  TEN_FREE(self);
}
