//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/extension_context/internal/add_extension.h"

#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_store/extension_store.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/mark.h"

void ten_extension_context_add_extension(void *self_, void *arg) {
  ten_extension_context_t *self = self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  ten_extension_t *added_extension = arg;
  TEN_ASSERT(added_extension, "Invalid argument.");

  ten_sanitizer_thread_check_inherit_from(&added_extension->thread_check,
                                          &self->thread_check);
  ten_sanitizer_thread_check_inherit_from(
      &added_extension->ten_env->thread_check, &self->thread_check);
  TEN_ASSERT(ten_extension_check_integrity(added_extension, true),
             "Should not happen.");

  TEN_UNUSED bool rc = ten_extension_store_add_extension(
      self->extension_store, added_extension, false);
  TEN_ASSERT(rc, "Should not happen.");

  // Find the target extension thread where the extension would be added to.
  ten_extension_thread_t *target_extension_thread =
      added_extension->extension_thread;
  TEN_ASSERT(target_extension_thread, "Invalid argument.");
  TEN_ASSERT(
      // TEN_NOLINTNEXTLINE(thread-check)
      // thread-check: We are in the engine thread. However, before the engine
      // is closed, the pointer of the extension group and the pointer of the
      // extension thread will not be changed, and the closing of the entire
      // engine must start from the engine, so the execution to this position
      // means that the engine has not been closed, so there will be no thread
      // safety issue.
      ten_extension_thread_check_integrity(target_extension_thread, false),
      "Invalid use of extension_thread %p.", target_extension_thread);

  ten_runloop_post_task_tail(
      ten_extension_get_attached_runloop(added_extension),
      ten_extension_thread_on_extension_added_to_engine,
      target_extension_thread, added_extension);
}
