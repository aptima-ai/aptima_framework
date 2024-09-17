//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/path/common.h"
#include "include_internal/ten_runtime/path/path_table.h"
#include "ten_utils/container/list.h"

// There is a possible group relationship among ten_path_t, and that group
// relationship representing the group of cmd results.
//
// There are 2 kinds of group of cmd results: one is related to the IN path
// table, and the other one is related to the OUT path table.
//
// - Relevant to the _OUT_ path table:
//   When 1 command maps to N commands when that command _leaves_ an extension.
//   That is the normal 1-to-N command mapping in TEN graphs. The OUT paths of
//   those N commands form a path group in the OUT path table.
//
//   Ex: The following is a 1-to-2 command mapping.
//   {
//     "app": "...",
//     "extension_group": "..."
//     "extension": "...",
//     "cmd": [{
//       "name": "hello world",
//       "dest": [{                       ==> 1
//         "app": "...",
//         "extension_group": "...",
//         "extension": "..."
//       },{                              ==> 2
//         "app": "...",
//         "extension_group": "...",
//         "extension": "..."
//       }]
//     }]
//   }
//
//   Such 1-to-N command mapping is used to trigger 1 operation of each
//   following extensions.
//
// - Relevant to the _IN_ path table:
//   When 1 command maps to N commands when that command _enters_ an extension.
//   That is the 1-to-N command mapping in the msg_conversions of TEN graphs.
//   The IN paths of those N commands form a path group in the IN path table.
//
//   Ex: The following is a 1-to-2 command mapping.
//   {
//     "app": "...",
//     "extension_group": "..."
//     "extension": "...",
//     "cmd": [{
//       "name": "hello world",
//       "dest": [{
//         "app": "...",
//         "extension_group": "...",
//         "extension": "...",
//         "msg_conversion": {
//           "type": "...",
//           "rules": [{
//             ...                   ==> 1
//           },{
//             ...                   ==> 2
//           }],
//           "result": {
//             ...
//           }
//         }
//       }]
//     }]
//   }
//
//   Such 1-to-N command mapping is used to trigger multiple operations of 1
//   following extension.
//
// The goal of a path group is to associate a 'logic' to the group of paths, and
// this logic can be easily declared in the JSON of graph, just like
// msg_conversions.
//
// The logic is to specify when will those paths being deleted, and a status
// command corresponding to those paths will be forwarded to the previous stage.
// The possible enum values are:
//
// - one_fail_return
//   If receives a fail cmd result, return that fail cmd result
//   immediately, and discard all the paths in the path group, and then discard
//   that path group.
//
// - all_ok_return_latest
//   If a cmd result in each path in the path group are received, and all
//   those cmd result is OK cmd results, then forward the latest
//   receiving cmd result to the previous stage, and discard all the paths
//   in the path group, and then discard that path group.
//
// - all_ok_return_oldest
//   Similar to the above one, but forward the oldest received cmd result.
//
// - all_return_latest
//   Similar to the above ones, do not care about the OK/Fail status in the
//   received cmd results, if all cmd results in the group are received,
//   return the latest received one.
//   *** And this is the current implement behavior.
//
// The whole process is like the following:
//
// 1. When a cmd result is forwarded through a path (whether it is an IN
//    path, or a OUT path), if the path corresponds to a result conversion
//    logic, the corresponding new cmd result will be generated according to
//    the settings of the result conversion setting.
//
// 2. When the "final" cmd result is got through the above step, if the path
//    is not in a path group, the handling of that cmd result will be the
//    same as it is now.
//
// 3. Otherwise (the path is in a path group), the default behavior is to store
// the cmd result into the path (ex: into 'cached_cmd_result' field) until
// the 'forward delivery conditions" of that path group is met.

#define TEN_PATH_GROUP_SIGNATURE 0x2EB016AECBDE782CU

typedef struct ten_path_t ten_path_t;
typedef struct ten_msg_conversion_operation_t ten_msg_conversion_operation_t;

typedef enum TEN_PATH_GROUP_ROLE {
  TEN_PATH_GROUP_ROLE_INVALID,

  TEN_PATH_GROUP_ROLE_MASTER,
  TEN_PATH_GROUP_ROLE_SLAVE,
} TEN_PATH_GROUP_ROLE;

typedef enum TEN_PATH_GROUP_POLICY {
  TEN_PATH_GROUP_POLICY_INVALID,

  // If receive a fail result, return it, otherwise, when all OK results are
  // received, return the first received one. Clear the group after returning
  // the result.
  TEN_PATH_GROUP_POLICY_ONE_FAIL_RETURN_AND_ALL_OK_RETURN_FIRST,

  // Similar to the above, except return the last received one.
  TEN_PATH_GROUP_POLICY_ONE_FAIL_RETURN_AND_ALL_OK_RETURN_LAST,

  // More modes is allowed, and could be added here in case needed.
} TEN_PATH_GROUP_POLICY;

typedef struct ten_path_group_t {
  ten_signature_t signature;
  ten_sanitizer_thread_check_t thread_check;

  ten_path_table_t *table;
  TEN_PATH_GROUP_ROLE role;

  union {
    // There should be only 1 master in a group.
    struct {
      TEN_PATH_GROUP_POLICY policy;
      ten_list_t members;  // Contain the members of the group.

      // If this flag is set, none of the paths in the path_group can be used to
      // trace back cmd results anymore.
      //
      // For example, if the policy is ONE_FAIL_RETURN_AND_ALL_OK_RETURN_FIRST
      // and one of the paths in the group has received a fail cmd result, then
      // the 'has_been_processed' flag will be set to true to prevent the left
      // paths in the group from transmitting cmd results.
      bool has_been_processed;
    } master;

    // There might be multiple slaves in a group.
    struct {
      ten_path_t *master;  // Point to the master of the group.
    } slave;
  };
} ten_path_group_t;

TEN_RUNTIME_PRIVATE_API bool ten_path_group_check_integrity(
    ten_path_group_t *self, bool check_thread);

TEN_RUNTIME_PRIVATE_API bool ten_path_is_in_a_group(ten_path_t *self);

TEN_RUNTIME_PRIVATE_API void ten_path_group_destroy(ten_path_group_t *self);

TEN_RUNTIME_PRIVATE_API ten_list_t *ten_path_group_get_members(
    ten_path_t *path);

TEN_RUNTIME_PRIVATE_API void ten_paths_create_group(
    ten_list_t *paths, TEN_PATH_GROUP_POLICY policy);

TEN_RUNTIME_PRIVATE_API ten_path_t *ten_path_group_resolve(ten_path_t *path,
                                                           TEN_PATH_TYPE type);

TEN_RUNTIME_PRIVATE_API ten_path_t *ten_path_group_get_master(ten_path_t *path);
