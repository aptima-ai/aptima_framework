//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg_conversion/msg_conversion_context.h"

#include "include_internal/axis_runtime/common/loc.h"
#include "include_internal/axis_runtime/extension/extension.h"
#include "include_internal/axis_runtime/extension/extension_info/extension_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg_conversion/msg_and_its_result_conversion.h"
#include "include_internal/axis_runtime/msg_conversion/msg_and_result_conversion.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/base.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/type.h"

bool axis_msg_conversion_context_check_integrity(
    axis_msg_conversion_context_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_MSG_CONVERSIONS_SIGNATURE) {
    return false;
  }
  return true;
}

axis_msg_conversion_context_t *axis_msg_conversion_context_create(
    const char *msg_name) {
  axis_ASSERT(msg_name, "Invalid argument.");

  axis_msg_conversion_context_t *self =
      (axis_msg_conversion_context_t *)axis_MALLOC(
          sizeof(axis_msg_conversion_context_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_MSG_CONVERSIONS_SIGNATURE);

  axis_string_init_formatted(&self->msg_name, "%s", msg_name);
  self->msg_and_result_conversion = NULL;

  return self;
}

void axis_msg_conversion_context_destroy(axis_msg_conversion_context_t *self) {
  axis_ASSERT(self && axis_msg_conversion_context_check_integrity(self),
             "Should not happen.");

  axis_string_deinit(&self->msg_name);
  axis_loc_deinit(&self->src_loc);
  if (self->msg_and_result_conversion) {
    axis_msg_and_result_conversion_destroy(self->msg_and_result_conversion);
  }

  axis_FREE(self);
}

static bool axis_msg_conversion_is_equal(axis_msg_conversion_context_t *a,
                                        axis_msg_conversion_context_t *b) {
  axis_ASSERT(a && axis_msg_conversion_context_check_integrity(a),
             "Should not happen.");
  axis_ASSERT(b && axis_msg_conversion_context_check_integrity(b),
             "Should not happen.");

  if (!axis_loc_is_equal(&a->src_loc, &b->src_loc)) {
    return false;
  }

  if (!axis_string_is_equal(&a->msg_name, &b->msg_name)) {
    return false;
  }

  return true;
}

static bool axis_msg_conversion_can_match_msg(axis_msg_conversion_context_t *self,
                                             axis_shared_ptr_t *msg) {
  axis_ASSERT(self && axis_msg_conversion_context_check_integrity(self),
             "Should not happen.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Should not happen.");

  const char *name = axis_msg_get_name(msg);
  axis_ASSERT(name, "Should not happen.");

  // @{
  // Because when we parse the graph declaration (ex: from JSON), we don't
  // know the graph ID at that time, so the graph ID in 'msg_conversions'
  // will be empty, so we need to update the correct graph ID here, so
  // that the following 'axis_loc_is_equal' could success.
  if (axis_string_is_empty(&self->src_loc.graph_id)) {
    axis_string_copy(&self->src_loc.graph_id,
                    &axis_msg_get_src_loc(msg)->graph_id);
  }
  // @}

  if (!axis_loc_is_equal(axis_msg_get_src_loc(msg), &self->src_loc) ||
      !axis_string_is_equal_c_str(&self->msg_name, name)) {
    return false;
  }

  return true;
}

bool axis_msg_conversion_context_merge(
    axis_list_t *msg_conversions,
    axis_msg_conversion_context_t *new_msg_conversion, axis_error_t *err) {
  axis_ASSERT(msg_conversions && axis_list_check_integrity(msg_conversions),
             "Should not happen.");
  axis_ASSERT(new_msg_conversion &&
                 axis_msg_conversion_context_check_integrity(new_msg_conversion),
             "Should not happen.");

  axis_list_foreach (msg_conversions, iter) {
    axis_msg_conversion_context_t *msg_conversion =
        axis_ptr_listnode_get(iter.node);
    axis_ASSERT(msg_conversion &&
                   axis_msg_conversion_context_check_integrity(msg_conversion),
               "Should not happen.");

    if (axis_msg_conversion_is_equal(msg_conversion, new_msg_conversion)) {
      if (err) {
        axis_error_set(err, axis_ERRNO_INVALID_GRAPH,
                      "Duplicated message conversion.");
      }
      axis_msg_conversion_context_destroy(new_msg_conversion);
      return false;
    }
  }

  axis_list_push_ptr_back(
      msg_conversions, new_msg_conversion,
      (axis_ptr_listnode_destroy_func_t)axis_msg_conversion_context_destroy);

  return true;
}

bool axis_extension_convert_msg(axis_extension_t *self, axis_shared_ptr_t *msg,
                               axis_list_t *result, axis_error_t *err) {
  axis_ASSERT(self && axis_extension_check_integrity(self, true),
             "Invalid argument");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");
  axis_ASSERT(result, "Should not happen.");
  axis_ASSERT(err && axis_error_check_integrity(err), "Invalid argument.");

  bool something_wrong = false;

  if (axis_msg_is_cmd_and_result(msg) &&
      axis_msg_get_type(msg) != axis_MSG_TYPE_CMD) {
    axis_error_set(err, axis_ERRNO_GENERIC, "Can not convert a builtin cmd.");
    something_wrong = true;
    goto done;
  }

  axis_ASSERT(
      axis_list_check_integrity(&self->extension_info->msg_conversion_contexts),
      "Should not happen.");

  axis_list_foreach (&self->extension_info->msg_conversion_contexts, iter) {
    axis_msg_conversion_context_t *msg_conversions =
        axis_ptr_listnode_get(iter.node);
    axis_ASSERT(msg_conversions &&
                   axis_msg_conversion_context_check_integrity(msg_conversions),
               "Should not happen.");

    // Find the correct message conversion function according to the current
    // source extension and the key of the message.
    if (axis_msg_conversion_can_match_msg(msg_conversions, msg)) {
      if (msg_conversions->msg_and_result_conversion) {
        axis_msg_conversion_t *msg_conversion =
            msg_conversions->msg_and_result_conversion->msg;
        axis_ASSERT(msg_conversion, "Invalid argument.");

        // Perform the message conversions.
        axis_shared_ptr_t *new_msg =
            axis_msg_conversion_convert(msg_conversion, msg, err);

        if (new_msg) {
          // Note: Although there might multiple messages been
          // converted/generated at once, and for a command, the command IDs of
          // those converted commands are equal, we do _not_ need to consider to
          // change the command IDs of those converted commands to different
          // values. Because those converted commands will be transmitted to an
          // extension, and just before entering that extension, APTIMA runtime
          // will add corresponding IN path into the IN path table, and at that
          // time, APTIMA runtime will detect there has already been a IN path in
          // the IN path table with the same command ID, and change the command
          // ID of the currently processed command to a different value at that
          // time.

          axis_list_push_ptr_back(
              result,
              axis_msg_and_its_result_conversion_create(
                  new_msg, msg_conversions->msg_and_result_conversion->result),
              (axis_ptr_listnode_destroy_func_t)
                  axis_msg_and_its_result_conversion_destroy);

          axis_shared_ptr_destroy(new_msg);
        } else {
          something_wrong = true;
        }
      }
    }
  }

  // If there is no matched message conversions, put the original message to
  // 'result'.
  if (axis_list_is_empty(result)) {
    axis_list_push_ptr_back(result,
                           axis_msg_and_its_result_conversion_create(msg, NULL),
                           (axis_ptr_listnode_destroy_func_t)
                               axis_msg_and_its_result_conversion_destroy);
  }

done:
  return !something_wrong;
}

static axis_msg_conversion_context_t *axis_msg_conversion_from_json_internal(
    axis_json_t *json, axis_loc_t *src_loc, const char *original_cmd_name,
    axis_error_t *err) {
  axis_ASSERT(json, "Invalid argument.");
  axis_ASSERT(original_cmd_name, "Invalid argument.");

  axis_msg_conversion_context_t *self =
      axis_msg_conversion_context_create(original_cmd_name);

  axis_loc_init_from_loc(&self->src_loc, src_loc);

  axis_ASSERT(axis_json_is_object(json), "Should not happen.");

  axis_msg_and_result_conversion_t *msg_result_operation =
      axis_msg_and_result_conversion_from_json(json, err);
  axis_ASSERT(msg_result_operation, "Should not happen.");
  if (!msg_result_operation) {
    axis_msg_conversion_context_destroy(self);
    return NULL;
  }

  self->msg_and_result_conversion = msg_result_operation;

  return self;
}

axis_msg_conversion_context_t *axis_msg_conversion_context_from_json(
    axis_json_t *json, axis_extension_info_t *src_extension_info,
    const char *cmd_name, axis_error_t *err) {
  axis_ASSERT(json && src_extension_info, "Should not happen.");

  return axis_msg_conversion_from_json_internal(json, &src_extension_info->loc,
                                               cmd_name, err);
}

static axis_msg_conversion_context_t *axis_msg_conversion_from_value_internal(
    axis_value_t *value, axis_loc_t *src_loc, const char *cmd_name,
    axis_error_t *err) {
  axis_ASSERT(value && cmd_name, "Should not happen.");

  axis_msg_conversion_context_t *self =
      axis_msg_conversion_context_create(cmd_name);

  axis_loc_init_from_loc(&self->src_loc, src_loc);

  axis_ASSERT(value->type == axis_TYPE_OBJECT, "Should not happen.");

  axis_msg_and_result_conversion_t *msg_result_operation =
      axis_msg_and_result_conversion_from_value(value, err);
  axis_ASSERT(msg_result_operation, "Should not happen.");
  if (!msg_result_operation) {
    axis_msg_conversion_context_destroy(self);
    return NULL;
  }

  self->msg_and_result_conversion = msg_result_operation;

  return self;
}

axis_msg_conversion_context_t *axis_msg_conversion_context_from_value(
    axis_value_t *value, axis_extension_info_t *src_extension_info,
    const char *cmd_name, axis_error_t *err) {
  axis_ASSERT(value && src_extension_info, "Should not happen.");

  return axis_msg_conversion_from_value_internal(value, &src_extension_info->loc,
                                                cmd_name, err);
}
