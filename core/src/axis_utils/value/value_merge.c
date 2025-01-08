//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_kv.h"

bool axis_value_object_merge_with_move(axis_value_t *dest, axis_value_t *src) {
  if (!dest || !src) {
    axis_ASSERT(0, "Invalid argument.");
    return false;
  }

  if (!axis_value_is_object(dest) || !axis_value_is_object(src)) {
    axis_ASSERT(0, "Invalid argument.");
    return false;
  }

  axis_value_object_foreach(src, src_iter) {
    // Detach the src_item from src.
    axis_list_detach_node(&src->content.object, src_iter.node);

    axis_value_kv_t *src_item = axis_ptr_listnode_get(src_iter.node);
    axis_ASSERT(src_item && axis_value_kv_check_integrity(src_item),
               "Should not happen.");

    bool found = false;

    axis_value_object_foreach(dest, dest_iter) {
      axis_value_kv_t *dest_item = axis_ptr_listnode_get(dest_iter.node);
      axis_ASSERT(dest_item && axis_value_kv_check_integrity(dest_item),
                 "Should not happen.");

      if (axis_string_is_equal(&src_item->key, &dest_item->key)) {
        if (axis_value_is_object(src_item->value) &&
            axis_value_is_object(dest_item->value)) {
          axis_value_object_merge_with_move(dest_item->value, src_item->value);
        } else {
          axis_value_kv_reset_to_value(dest_item, src_item->value);
          src_item->value = NULL;
        }

        found = true;
      }
    }

    if (found) {
      axis_listnode_destroy(src_iter.node);
    } else {
      // Move the src_item to dest.
      axis_list_push_back(&dest->content.object, src_iter.node);
    }
  }

  return true;
}

/**
 * @brief Merge value from @a src to @a dest.
 */
bool axis_value_object_merge_with_clone(axis_value_t *dest, axis_value_t *src) {
  if (!dest || !src) {
    axis_ASSERT(0, "Invalid argument.");
    return false;
  }

  if (!axis_value_is_object(dest) || !axis_value_is_object(src)) {
    axis_ASSERT(0, "Invalid argument.");
    return false;
  }

  // Iterate over the source object.
  axis_value_object_foreach(src, src_iter) {
    axis_value_kv_t *src_item = axis_ptr_listnode_get(src_iter.node);
    axis_ASSERT(src_item && axis_value_kv_check_integrity(src_item),
               "Should not happen.");

    bool found = false;

    // Iterate over the destination object to find a matching key.
    axis_value_object_foreach(dest, dest_iter) {
      axis_value_kv_t *dest_item = axis_ptr_listnode_get(dest_iter.node);
      axis_ASSERT(dest_item && axis_value_kv_check_integrity(dest_item),
                 "Should not happen.");

      if (axis_string_is_equal(&src_item->key, &dest_item->key)) {
        found = true;

        // If both values are objects, merge them recursively.
        if (axis_value_is_object(src_item->value) &&
            axis_value_is_object(dest_item->value)) {
          if (!axis_value_object_merge_with_clone(dest_item->value,
                                                 src_item->value)) {
            return false;
          }
        } else {
          // Otherwise, clone the source value and replace the destination
          // value.
          axis_value_t *new_src = axis_value_clone(src_item->value);
          axis_ASSERT(new_src, "Should not happen.");

          axis_value_kv_reset_to_value(dest_item, new_src);
        }

        break;
      }
    }

    // If no matching key was found, clone the source item and add it to the
    // destination.
    if (!found) {
      axis_value_kv_t *new_src = axis_value_kv_clone(src_item);
      axis_ASSERT(new_src, "Should not happen.");

      axis_list_push_ptr_back(
          &dest->content.object, new_src,
          (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
    }
  }

  return true;
}

bool axis_value_object_merge_with_json(axis_value_t *dest, axis_json_t *src) {
  if (!dest || !src) {
    axis_ASSERT(0, "Invalid argument.");
    return false;
  }

  if (!axis_value_is_object(dest) || !axis_json_is_object(src)) {
    axis_ASSERT(0, "Invalid argument.");
    return false;
  }

  const char *key = NULL;
  axis_json_t *prop_json = NULL;
  axis_json_object_foreach(src, key, prop_json) {
    bool found = false;

    axis_value_object_foreach(dest, dest_iter) {
      axis_value_kv_t *dest_item = axis_ptr_listnode_get(dest_iter.node);
      axis_ASSERT(dest_item && axis_value_kv_check_integrity(dest_item),
                 "Should not happen.");

      if (axis_string_is_equal_c_str(&dest_item->key, key)) {
        if (axis_json_is_object(prop_json) &&
            axis_value_is_object(dest_item->value)) {
          axis_value_object_merge_with_json(dest_item->value, prop_json);
        } else {
          axis_value_t *src_value = axis_value_from_json(prop_json);
          axis_ASSERT(src_value, "Should not happen.");

          axis_value_kv_reset_to_value(dest_item, src_value);
        }

        found = true;
      }
    }

    if (!found) {
      // Clone the src_item to dest.
      axis_value_kv_t *src_kv =
          axis_value_kv_create(key, axis_value_from_json(prop_json));
      axis_ASSERT(src_kv, "Should not happen.");

      axis_list_push_ptr_back(
          &dest->content.object, src_kv,
          (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
    }
  }

  return true;
}
