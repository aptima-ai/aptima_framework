//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/extension_context/internal/del_extension.h"

#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_store/extension_store.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/check.h"

void ten_extension_context_delete_extension(void *self_, void *arg) {
  ten_extension_context_t *self = self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  ten_extension_t *deleted_extension = arg;
  TEN_ASSERT(deleted_extension, "Invalid argument.");

  ten_sanitizer_thread_check_inherit_from(&deleted_extension->thread_check,
                                          &self->thread_check);
  ten_sanitizer_thread_check_inherit_from(
      &deleted_extension->ten_env->thread_check, &self->thread_check);
  TEN_ASSERT(ten_extension_check_integrity(deleted_extension, true),
             "Should not happen.");

  TEN_LOGD("[%s] Engine removes %s from its context",
           ten_engine_get_name(self->engine),
           ten_string_get_raw_str(&deleted_extension->name));

  ten_extension_store_del_extension(self->extension_store, deleted_extension,
                                    false);

  // Send del_extension command to the corresponding extension thread.
  ten_extension_thread_t *target_extension_thread =
      deleted_extension->extension_thread;
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: when we are here, it means the engine is still alive, not
  // closing, so the extension thread will be alive, too. And the runloop of the
  // extension thread will not be changed after it has been created, so it's
  // safe to access it here.
  TEN_ASSERT(target_extension_thread && ten_extension_thread_check_integrity(
                                            target_extension_thread, false),
             "Should not happen.");

  ten_runloop_post_task_tail(
      ten_extension_get_attached_runloop(deleted_extension),
      ten_extension_thread_on_extension_deleted_from_engine,
      target_extension_thread, deleted_extension);
}
