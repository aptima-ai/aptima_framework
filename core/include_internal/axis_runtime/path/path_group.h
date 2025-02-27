//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/path/common.h"
#include "include_internal/axis_runtime/path/path_table.h"
#include "include_internal/axis_runtime/path/result_return_policy.h"
#include "axis_utils/container/list.h"

// There is a possible group relationship among axis_path_t, and that group
// relationship representing the group of cmd results.
//
// There are 2 kinds of group of cmd results: one is related to the IN path
// table, and the other one is related to the OUT path table.
//
// - Relevant to the _OUT_ path table:
//   When 1 command maps to N commands when that command _leaves_ an extension.
//   That is the normal 1-to-N command mapping in APTIMA graphs. The OUT paths of
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
//   That is the 1-to-N command mapping in the msg_conversions of APTIMA graphs.
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

#define axis_PATH_GROUP_SIGNATURE 0x2EB016AECBDE782CU

typedef struct axis_path_t axis_path_t;
typedef struct axis_msg_conversion_t axis_msg_conversion_t;

typedef struct axis_path_group_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_path_table_t *table;

  axis_RESULT_RETURN_POLICY policy;

  // Contain the members of the group.
  axis_list_t members;  // axis_path_t
} axis_path_group_t;

axis_RUNTIME_PRIVATE_API bool axis_path_group_check_integrity(
    axis_path_group_t *self, bool check_thread);

axis_RUNTIME_PRIVATE_API bool axis_path_is_in_a_group(axis_path_t *self);

axis_RUNTIME_PRIVATE_API void axis_path_group_destroy(axis_path_group_t *self);

axis_RUNTIME_PRIVATE_API axis_list_t *axis_path_group_get_members(
    axis_path_t *path);

axis_RUNTIME_PRIVATE_API void axis_paths_create_group(
    axis_list_t *paths, axis_RESULT_RETURN_POLICY policy);

axis_RUNTIME_PRIVATE_API axis_path_t *axis_path_group_resolve(axis_path_t *path,
                                                           axis_PATH_TYPE type);
