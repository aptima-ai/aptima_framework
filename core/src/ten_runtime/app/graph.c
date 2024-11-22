//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/app/graph.h"

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/base_dir.h"
#include "ten_runtime/app/app.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/mark.h"

#if defined(TEN_ENABLE_TEN_RUST_APIS)
#include "include_internal/ten_runtime/common/constant_str.h"
#include "include_internal/ten_rust/ten_rust.h"
#include "ten_utils/macro/memory.h"
#endif

bool ten_app_check_start_graph_cmd(ten_app_t *self,
                                   ten_shared_ptr_t *start_graph_cmd,
                                   TEN_UNUSED ten_error_t *err) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");
  TEN_ASSERT(start_graph_cmd, "Invalid argument.");

#if defined(TEN_ENABLE_TEN_RUST_APIS)
  const char *base_dir = ten_app_get_base_dir(self);

  // The pkg_info of extensions in the graph is read from the ten_packages
  // directory under the base dir of app. If the base dir is not set, the app
  // might be running in a thread, ex: the smoke testing. In this case, we can
  // not retrieve the enough information to check the graph.
  if (!base_dir || ten_c_string_is_empty(base_dir)) {
    TEN_LOGD("The base dir of app [%s] is not set, skip checking graph.",
             ten_app_get_uri(self));
    return true;
  }

  ten_json_t *start_graph_cmd_json = ten_msg_to_json(start_graph_cmd, err);
  if (!start_graph_cmd_json) {
    TEN_ASSERT(0,
               "Failed to convert start graph cmd to json, should not happen.");
    return false;
  }

  bool free_json_string = false;
  const char *graph_json_str = ten_json_to_string(
      start_graph_cmd_json, TEN_STR_UNDERLINE_TEN, &free_json_string);

  const char *err_msg = NULL;
  bool rc = ten_rust_check_graph_for_app(base_dir, graph_json_str,
                                         ten_app_get_uri(self), &err_msg);

  if (free_json_string) {
    TEN_FREE(graph_json_str);
  }

  if (!rc) {
    ten_error_set(err, TEN_ERRNO_INVALID_GRAPH, err_msg);
    ten_rust_free_cstring(err_msg);
  }

  ten_json_destroy(start_graph_cmd_json);

  return rc;
#else
  return true;
#endif
}
