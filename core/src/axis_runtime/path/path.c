//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/path/path.h"

#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/base.h"
#include "include_internal/axis_runtime/path/path_group.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/sanitizer/thread_check.h"

bool axis_path_check_integrity(axis_path_t *self, bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_PATH_SIGNATURE) {
    return false;
  }

  if (check_thread) {
    // In case the extension thread may be lock_mode, we utilize
    // extension_thread_check_integrity to handle this scenario.
    if (self->table->attach_to == axis_PATH_TABLE_ATTACH_TO_EXTENSION) {
      axis_extension_thread_t *extension_thread =
          self->table->attached_target.extension->extension_thread;
      return axis_extension_thread_check_integrity(extension_thread, true);
    }

    return axis_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

void axis_path_init(axis_path_t *self, axis_path_table_t *table,
                   axis_PATH_TYPE type, const char *cmd_name,
                   const char *parent_cmd_id, const char *cmd_id,
                   axis_loc_t *src_loc, axis_loc_t *dest_loc) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(cmd_name && strlen(cmd_name), "Invalid argument.");

  axis_signature_set(&self->signature, axis_PATH_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  self->table = table;
  self->type = type;

  axis_string_init_formatted(&self->cmd_name, "%s", cmd_name);

  if (parent_cmd_id) {
    axis_string_init_formatted(&self->original_cmd_id, "%s", parent_cmd_id);
  } else {
    axis_string_init(&self->original_cmd_id);
  }
  axis_string_init_formatted(&self->cmd_id, "%s", cmd_id);

  axis_loc_init_from_loc(&self->src_loc, src_loc);
  axis_loc_init_from_loc(&self->dest_loc, dest_loc);

  self->group = NULL;
  self->cached_cmd_result = NULL;
  self->result_conversion = NULL;
  self->expired_time_us = UINT64_MAX;
}

void axis_path_deinit(axis_path_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_NOLINTNEXTLINE(thread-check)
  // thread-check: The belonging thread might be destroyed, so we do not check
  // the thread safety here.
  axis_ASSERT(axis_path_check_integrity(self, false), "Should not happen.");

  axis_string_deinit(&self->cmd_name);

  axis_string_deinit(&self->cmd_id);
  axis_string_deinit(&self->original_cmd_id);

  axis_loc_deinit(&self->src_loc);
  axis_loc_deinit(&self->dest_loc);

  if (self->group) {
    axis_shared_ptr_destroy(self->group);
    self->group = NULL;
  }

  if (self->cached_cmd_result) {
    axis_shared_ptr_destroy(self->cached_cmd_result);
    self->cached_cmd_result = NULL;
  }

  axis_sanitizer_thread_check_deinit(&self->thread_check);
  axis_signature_set(&self->signature, 0);
}

void axis_path_set_result(axis_path_t *path, axis_shared_ptr_t *cmd_result) {
  axis_ASSERT(path && axis_path_check_integrity(path, true), "Invalid argument.");
  axis_ASSERT(cmd_result &&
                 axis_msg_get_type(cmd_result) == axis_MSG_TYPE_CMD_RESULT &&
                 axis_cmd_base_check_integrity(cmd_result),
             "Invalid argument.");

  if (path->cached_cmd_result) {
    // This situation happened at the time below.
    //
    // 1. The cmd result is sent to the extension, and that cmd result
    //    corresponds to a command which came from the previous node, so APTIMA
    //    runtime will cache the cmd result to the IN path, too.
    //
    //      <-- IN path <-- extension <-- OUT path
    //
    // 2. The extension handles that cmd result, and explicitly call
    //    return_xxx() for that cmd result in the extension's on_cmd(). APTIMA
    //    runtime will cache the cmd result to the IN path in this case. The
    //    extension might modify the cmd result (ex: add more properties),
    //    so we must replace the original cached cmd result here.

    // NOTE: We cannot access the contents of `cached_cmd_result` here because
    // it might already be in the `in path_table` of another extension in
    // another extension thread. Accessing `cached_cmd_result` here could lead
    // to a thread safety issue.

    // axis_msg_dump(cmd_result, NULL, "The new cached cmd result: ^m");

    axis_shared_ptr_destroy(path->cached_cmd_result);
    path->cached_cmd_result = NULL;
  }

  if (path->result_conversion) {
    // If there is a cmd result conversion setting, use it.

    axis_error_t err;
    axis_error_init(&err);

    cmd_result =
        axis_msg_conversion_convert(path->result_conversion, cmd_result, &err);
    if (!cmd_result) {
      axis_LOGE("Failed to convert cmd result: %s", axis_error_errmsg(&err));

      // Since the flow of the cmd result must not be interrupted, otherwise
      // the extension, which expects to receive the cmd result, will not
      // receive it and will hang. Therefore, if converting the cmd result
      // fails here, there will be a fallback path, which is to clone the
      // original cmd result, allowing the extension to receive 'what may be
      // an incorrect cmd result'. This at least ensures that the status
      // command is recognized as incorrect, enabling users to check which part
      // of the conversion rule is problematic.
      cmd_result = axis_shared_ptr_clone(cmd_result);
    }

    axis_error_deinit(&err);
  }

  path->cached_cmd_result = axis_shared_ptr_clone(cmd_result);

  if (path->result_conversion) {
    axis_shared_ptr_destroy(cmd_result);
    cmd_result = NULL;
  }

  if (axis_path_is_in_a_group(path)) {
    // Move the current path to the last of the members of the group, so that we
    // can know which one should be returned in different policies.
    axis_path_group_t *path_group = axis_path_get_group(path);

    axis_list_t *members = &path_group->members;
    axis_ASSERT(members, "Should not happen.");

    axis_listnode_t *path_node = axis_list_find_ptr(members, path);
    axis_ASSERT(path_node, "A path should be found, otherwise, it is a bug.");

    axis_list_detach_node(members, path_node);
    axis_list_push_back(members, path_node);
  }
}

void axis_path_set_expired_time(axis_path_t *path, uint64_t expired_time_us) {
  axis_ASSERT(path && axis_path_check_integrity(path, true), "Invalid argument.");

  path->expired_time_us = expired_time_us;
}

axis_path_group_t *axis_path_get_group(axis_path_t *self) {
  axis_ASSERT(self && axis_path_check_integrity(self, true), "Invalid argument.");

  axis_path_group_t *path_group =
      (axis_path_group_t *)axis_shared_ptr_get_data(self->group);
  axis_ASSERT(path_group && axis_path_group_check_integrity(path_group, true),
             "Invalid argument.");

  return path_group;
}
