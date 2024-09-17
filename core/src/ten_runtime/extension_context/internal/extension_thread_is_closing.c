//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/extension_context/internal/extension_thread_is_closing.h"

#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node_ptr.h"

void ten_extension_context_on_extension_thread_closing_flag_is_set(void *self_,
                                                                   void *arg) {
  ten_extension_context_t *self = self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_extension_context_check_integrity(self, true),
             "Invalid use of extension_context %p.", self);

  ten_extension_group_t *extension_group = arg;
  TEN_ASSERT(extension_group &&
                 // TEN_NOLINTNEXTLINE(thread-check)
                 // thread-check: We only access the read-only fields of the
                 // extension_group in this function, so it's safe to use it in
                 // the engine thread.
                 ten_extension_group_check_integrity(extension_group, false),
             "Should not happen.");

  TEN_LOGD("[%s] Engine is notified that %s is_closing",
           ten_engine_get_name(self->engine),
           ten_string_get_raw_str(&extension_group->name));

  self->extension_threads_cnt_of_closing_flag_is_set++;

  if (self->extension_threads_cnt_of_closing_flag_is_set ==
      ten_list_size(&self->extension_threads)) {
    TEN_LOGD("[%s] All extension threads enter 'closing' state.",
             ten_engine_get_name(self->engine));

    ten_list_foreach (&self->extension_threads, iter) {
      ten_extension_thread_t *thread = ten_ptr_listnode_get(iter.node);
      // TEN_NOLINTNEXTLINE(thread-check)
      // thread-check: We only access the read-only fields of the
      // extension_thread in this function, so it's safe to use it in
      // the engine thread.
      TEN_ASSERT(thread && ten_extension_thread_check_integrity(thread, false),
                 "Should not happen.");

      ten_runloop_post_task_tail(
          ten_extension_group_get_attached_runloop(thread->extension_group),
          ten_extension_thread_call_all_extensions_on_deinit, thread, NULL);
    }
  }
}
