//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/graph.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/base_dir.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/mark.h"

#if defined(axis_ENABLE_axis_RUST_APIS)
#include "include_internal/axis_runtime/common/constant_str.h"
#include "include_internal/axis_rust/axis_rust.h"
#include "axis_utils/macro/memory.h"
#endif

bool axis_app_check_start_graph_cmd(axis_app_t *self,
                                   axis_shared_ptr_t *start_graph_cmd,
                                   axis_UNUSED axis_error_t *err) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");
  axis_ASSERT(start_graph_cmd, "Invalid argument.");

#if defined(axis_ENABLE_axis_RUST_APIS)
  const char *base_dir = axis_app_get_base_dir(self);

  // The pkg_info of extensions in the graph is read from the axis_packages
  // directory under the base dir of app. If the base dir is not set, the app
  // might be running in a thread, ex: the smoke testing. In this case, we can
  // not retrieve the enough information to check the graph.
  if (!base_dir || axis_c_string_is_empty(base_dir)) {
    axis_LOGD("The base dir of app [%s] is not set, skip checking graph.",
             axis_app_get_uri(self));
    return true;
  }

  axis_json_t *start_graph_cmd_json =
      axis_msg_to_json_include_internal_field(start_graph_cmd, err);
  if (!start_graph_cmd_json) {
    axis_ASSERT(0,
               "Failed to convert start graph cmd to json, should not happen.");
    return false;
  }

  bool free_json_string = false;
  const char *graph_json_str = axis_json_to_string(
      start_graph_cmd_json, axis_STR_UNDERLINE_TEN, &free_json_string);

  const char *err_msg = NULL;
  bool rc = axis_rust_check_graph_for_app(base_dir, graph_json_str,
                                         axis_app_get_uri(self), &err_msg);

  if (free_json_string) {
    axis_FREE(graph_json_str);
  }

  if (!rc) {
    axis_error_set(err, axis_ERRNO_INVALID_GRAPH, err_msg);
    axis_rust_free_cstring(err_msg);
  }

  axis_json_destroy(start_graph_cmd_json);

  return rc;
#else
  return true;
#endif
}
