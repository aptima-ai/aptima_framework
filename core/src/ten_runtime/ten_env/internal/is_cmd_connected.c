//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/app/ten_env/metadata.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension/metadata.h"
#include "include_internal/ten_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "include_internal/ten_runtime/extension/ten_env/metadata.h"
#include "include_internal/ten_runtime/extension_context/extension_context.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_group/ten_env/metadata.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/ten_env/metadata_cb.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/check.h"
#include "ten_runtime/app/app.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/extension/extension.h"
#include "ten_runtime/ten_env/internal/metadata.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/macro/mark.h"

typedef struct ten_is_cmd_connected_context_t {
  ten_string_t name;
  ten_env_is_cmd_connected_async_cb_t cb;
  void *cb_data;
  bool result;
} ten_is_cmd_connected_context_t;

static ten_is_cmd_connected_context_t *ten_is_cmd_connected_context_create(
    const char *name, ten_env_is_cmd_connected_async_cb_t cb, void *cb_data) {
  ten_is_cmd_connected_context_t *context =
      TEN_MALLOC(sizeof(ten_is_cmd_connected_context_t));
  TEN_ASSERT(context, "Failed to allocate memory.");

  ten_string_init_formatted(&context->name, name);
  context->cb = cb;
  context->cb_data = cb_data;
  context->result = true;

  return context;
}

static void ten_is_cmd_connected_context_destroy(
    ten_is_cmd_connected_context_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_string_deinit(&self->name);

  TEN_FREE(self);
}

bool ten_env_is_cmd_connected(ten_env_t *self, const char *cmd_name,
                              TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_env_check_integrity(self, true), "Invalid use of ten_env %p.",
             self);
  TEN_ASSERT(cmd_name, "Should not happen.");

  if (!ten_c_string_is_empty(cmd_name)) {
    ten_listnode_t *cmd_dest_node = ten_list_find_shared_ptr_custom(
        &ten_env_get_attached_extension(self)->msg_dest_runtime_info.cmd,
        cmd_name, ten_msg_dest_runtime_info_qualified);
    if (cmd_dest_node) {
      return true;
    }
  }

  return false;
}

static void ten_is_cmd_connected_task(void *self_, void *arg) {
  ten_env_t *self = (ten_env_t *)self_;
  TEN_ASSERT(self && ten_env_check_integrity(self, true), "Should not happen.");

  ten_is_cmd_connected_context_t *context =
      (ten_is_cmd_connected_context_t *)arg;
  TEN_ASSERT(context, "Should not happen.");

  ten_error_t err;
  ten_error_init(&err);

  context->result = ten_env_is_cmd_connected(
      self, ten_string_get_raw_str(&context->name), &err);

  if (context->cb) {
    context->cb(self, context->result, context->cb_data, &err);
  }

  ten_error_deinit(&err);
  ten_is_cmd_connected_context_destroy(context);
}

bool ten_is_cmd_connected_async(ten_env_t *self, const char *cmd_name,
                                ten_env_is_cmd_connected_async_cb_t cb,
                                void *cb_data, ten_error_t *err) {
  TEN_ASSERT(self && ten_env_check_integrity(self, true), "Invalid argument.");

  if (self->attach_to != TEN_ENV_ATTACH_TO_EXTENSION) {
    if (err) {
      ten_error_set(err, TEN_ERRNO_INVALID_ARGUMENT, "Invalid operation.");
    }
    return false;
  }

  ten_extension_t *extension = self->attached_target.extension;
  TEN_ASSERT(extension && ten_extension_check_integrity(extension, true),
             "Should not happen.");

  ten_extension_thread_t *extension_thread = extension->extension_thread;
  TEN_ASSERT(extension_thread &&
                 ten_extension_thread_check_integrity(extension_thread, true),
             "Should not happen.");

  ten_is_cmd_connected_context_t *context =
      ten_is_cmd_connected_context_create(cmd_name, cb, cb_data);

  ten_runloop_post_task_tail(extension_thread->runloop,
                             ten_is_cmd_connected_task, self, context);

  return true;
}
