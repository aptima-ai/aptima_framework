//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/path/path_group.h"

#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension_thread/extension_thread.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/path/path.h"
#include "axis_runtime/common/status_code.h"
#include "axis_runtime/msg/cmd_result/cmd_result.h"
#include "axis_runtime/msg/msg.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_ptr.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

bool axis_path_group_check_integrity(axis_path_group_t *self, bool check_thread) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_PATH_GROUP_SIGNATURE) {
    return false;
  }

  if (check_thread) {
    // In case the extension thread may be in lock_mode, we utilize
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

/**
 * @brief Checks whether a given path is associated with a group or not.
 *
 * @return true if the path is part of a group, otherwise, it returns false.
 */
bool axis_path_is_in_a_group(axis_path_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  return self->group ? true : false;
}

/**
 * @brief The three functions @name axis_path_group_create, @name
 * axis_path_group_create_master, and @name axis_path_group_create_slave create a
 * new path group, a master group, and a slave group, respectively. They
 * allocate memory for the structure, initialize various components of the path
 * group, and return the group.
 */
static axis_path_group_t *axis_path_group_create(
    axis_path_table_t *table, axis_RESULT_RETURN_POLICY policy) {
  axis_path_group_t *self =
      (axis_path_group_t *)axis_MALLOC(sizeof(axis_path_group_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature, axis_PATH_GROUP_SIGNATURE);
  axis_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  self->table = table;
  self->policy = policy;
  axis_list_init(&self->members);

  return self;
}

/**
 * @brief Frees the memory allocated for the path group.
 */
void axis_path_group_destroy(axis_path_group_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_sanitizer_thread_check_deinit(&self->thread_check);
  axis_signature_set(&self->signature, 0);

  axis_list_clear(&self->members);

  axis_FREE(self);
}

/**
 * @brief Creates a group of paths. It takes a list of paths as an argument and
 * initializes each path with a group. The first path in the list is designated
 * as the master, and the rest are designated as slaves. It also initializes the
 * members of the master group.
 */
void axis_paths_create_group(axis_list_t *paths,
                            axis_RESULT_RETURN_POLICY policy) {
  axis_ASSERT(paths, "Invalid argument.");
  axis_ASSERT(axis_list_size(paths) > 1, "Invalid argument.");

  axis_path_group_t *path_group = NULL;
  axis_shared_ptr_t *path_group_sp = NULL;

  axis_list_foreach (paths, iter) {
    axis_path_t *path = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(path && axis_path_check_integrity(path, true),
               "Invalid argument.");
    axis_ASSERT(path->table, "Invalid argument.");

    if (!path_group_sp) {
      path_group = axis_path_group_create(path->table, policy);
      path_group_sp = axis_shared_ptr_create(path_group, axis_path_group_destroy);
      path->group = path_group_sp;
    } else {
      path->group = axis_shared_ptr_clone(path_group_sp);
    }

    axis_list_push_ptr_back(&path_group->members, path, NULL);
  }
}

/**
 * @brief Takes a list of paths and a path type as arguments and checks the
 * status of each path in the group. If any of the paths fail, it returns that
 * path, otherwise, it returns the first or last path in the list, depending on
 * the parameter `return_last`. This function is used to resolve the status of a
 * group of paths.
 */
static axis_path_t *axis_path_group_resolve_in_one_fail_and_all_ok_return(
    axis_list_t *members, axis_UNUSED axis_PATH_TYPE path_type, bool return_last) {
  axis_ASSERT(members && axis_list_check_integrity(members), "Invalid argument.");

  bool has_received_all_cmd_results_in_group = true;

  axis_list_foreach (members, iter) {
    axis_path_t *path = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(path && axis_path_check_integrity(path, true),
               "Invalid argument.");

    axis_shared_ptr_t *cmd_result = path->cached_cmd_result;

    if (cmd_result) {
      axis_ASSERT(axis_msg_get_type(cmd_result) == axis_MSG_TYPE_CMD_RESULT,
                 "Invalid argument.");

      if (axis_cmd_result_get_status_code(cmd_result) != axis_STATUS_CODE_OK) {
        // Receive a fail result, return it.
        return path;
      }
    } else {
      has_received_all_cmd_results_in_group = false;
    }
  }

  if (has_received_all_cmd_results_in_group) {
    // Receive all cmd results, the purpose of the group is completed, too.
    // Therefore, return the specified cmd result.
    axis_path_t *path = axis_ptr_listnode_get(
        return_last ? axis_list_back(members) : axis_list_front(members));
    axis_ASSERT(path && axis_path_check_integrity(path, true),
               "Invalid argument.");
    axis_ASSERT(path->cached_cmd_result, "Should not happen.");

    return path;
  }

  return NULL;
}

/**
 * @brief Takes a path as an argument and returns a list of all the paths that
 * belong to the same group as the given path.
 */
axis_list_t *axis_path_group_get_members(axis_path_t *path) {
  axis_ASSERT(path && axis_path_check_integrity(path, true), "Invalid argument.");
  axis_ASSERT(axis_path_is_in_a_group(path), "Invalid argument.");

  axis_path_group_t *path_group = axis_path_get_group(path);

  axis_list_t *members = &path_group->members;
  axis_ASSERT(members && axis_list_check_integrity(members),
             "Should not happen.");

  return members;
}

/**
 * @brief Resolve the path group, where @a path belongs to, to a final cmd
 * result.
 *
 * This function takes a path and a path type as arguments and resolves the path
 * group to a final cmd result. It checks the policy of the path group and calls
 * the appropriate function to resolve the path group status.
 *
 * Policy `axis_RESULT_RETURN_POLICY_FIRST_ERROR_OR_FIRST_OK` returns the first
 * path in the list if all paths have succeeded, and returns the first path that
 * has failed otherwise.
 */
axis_path_t *axis_path_group_resolve(axis_path_t *path, axis_PATH_TYPE type) {
  axis_ASSERT(path && axis_path_check_integrity(path, true), "Invalid argument.");
  axis_ASSERT(axis_path_is_in_a_group(path), "Invalid argument.");

  axis_path_group_t *path_group = axis_path_get_group(path);

  axis_list_t *members = &path_group->members;
  axis_ASSERT(members && axis_list_check_integrity(members),
             "Should not happen.");

  switch (path_group->policy) {
    case axis_RESULT_RETURN_POLICY_FIRST_ERROR_OR_FIRST_OK:
      return axis_path_group_resolve_in_one_fail_and_all_ok_return(members, type,
                                                                  false);
    case axis_RESULT_RETURN_POLICY_FIRST_ERROR_OR_LAST_OK:
      return axis_path_group_resolve_in_one_fail_and_all_ok_return(members, type,
                                                                  true);
    case axis_RESULT_RETURN_POLICY_EACH_OK_AND_ERROR:
      // In this policy, we return the current path immediately.
      return path;
    default:
      axis_ASSERT(0, "Should not happen.");
      return NULL;
  }

  axis_ASSERT(0, "Should not happen.");
}
