//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/value/value_path.h"

#include <stdlib.h>

#include "include_internal/axis_utils/value/constant_str.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/container/list_node_ptr.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_kv.h"

typedef enum axis_VALUE_PATH_SNIPPET_ITEM_TYPE {
  axis_VALUE_PATH_SNIPPET_ITEM_TYPE_INVALID,

  axis_VALUE_PATH_SNIPPET_ITEM_TYPE_STRING,
  axis_VALUE_PATH_SNIPPET_ITEM_TYPE_INDEX,
} axis_VALUE_PATH_SNIPPET_ITEM_TYPE;

typedef struct axis_value_path_snippet_item_t {
  axis_VALUE_PATH_SNIPPET_ITEM_TYPE type;

  union {
    axis_string_t str;
    size_t index;
  };
} axis_value_path_snippet_item_t;

static void axis_value_path_snippet_item_destroy(
    axis_value_path_snippet_item_t *item) {
  axis_ASSERT(item, "Invalid argument.");

  switch (item->type) {
    case axis_VALUE_PATH_SNIPPET_ITEM_TYPE_STRING:
      axis_string_deinit(&item->str);
      break;

    default:
      break;
  }

  axis_FREE(item);
}

static axis_value_path_item_t *axis_value_path_item_create(void) {
  axis_value_path_item_t *item =
      (axis_value_path_item_t *)axis_MALLOC(sizeof(axis_value_path_item_t));
  axis_ASSERT(item, "Failed to allocate memory.");

  // Initialize all memory within `item` to 0, so that the type within it (such
  // as `axis_string_t`) recognizes it as being in an uninitialized state.
  memset(item, 0, sizeof(axis_value_path_item_t));

  item->type = axis_VALUE_PATH_ITEM_TYPE_INVALID;

  return item;
}

static void axis_value_path_item_destroy(axis_value_path_item_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  switch (self->type) {
    case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM:
      axis_string_deinit(&self->obj_item_str);
      break;

    default:
      break;
  }

  axis_FREE(self);
}

static axis_value_path_item_t *axis_value_path_parse_between_bracket(
    axis_string_t *content, bool is_first) {
  axis_value_path_item_t *item = axis_value_path_item_create();

  if (is_first) {
    item->type = axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM;
    axis_string_init_from_string(&item->obj_item_str, content);
  } else {
    if (!axis_c_string_is_equal(
            axis_string_get_raw_str(content) + axis_string_len(content) - 1,
            axis_STR_VALUE_PATH_ARRAY_END)) {
      // It's not a valid array specifier, i.e., starting from '[', but not
      // ended with ']'.
      goto error;
    } else {
      axis_string_erase_back(content, 1);

      if (axis_c_string_is_equal(
              axis_string_get_raw_str(content) + axis_string_len(content) - 1,
              axis_STR_VALUE_PATH_ARRAY_END)) {
        // It's not a valid array specifier, i.e., starting from '[', but
        // ended with multiple ']'.
        goto error;
      }
    }

    item->type = axis_VALUE_PATH_ITEM_TYPE_ARRAY_ITEM;
    item->arr_idx = strtol(axis_string_get_raw_str(content), NULL, 10);
  }

  return item;

error:
  axis_value_path_item_destroy(item);
  return NULL;
}

static bool axis_value_path_parse_between_colon(axis_string_t *path,
                                               axis_list_t *result) {
  axis_ASSERT(path, "Invalid argument.");
  axis_ASSERT(result, "Invalid argument.");

  bool rc = true;

  axis_list_t split_by_bracket = axis_LIST_INIT_VAL;
  axis_string_split(path, axis_STR_VALUE_PATH_ARRAY_START, &split_by_bracket);

  axis_list_foreach (&split_by_bracket, iter) {
    axis_string_t *content_between_bracket = axis_str_listnode_get(iter.node);
    axis_ASSERT(content_between_bracket, "Invalid argument.");

    axis_value_path_item_t *item = axis_value_path_parse_between_bracket(
        content_between_bracket, iter.index == 0 ? true : false);

    if (!item) {
      rc = false;
      goto done;
    }

    axis_list_push_ptr_back(
        result, item,
        (axis_ptr_listnode_destroy_func_t)axis_value_path_item_destroy);
  }

done:
  axis_list_clear(&split_by_bracket);
  return rc;
}

bool axis_value_path_parse(const char *path, axis_list_t *result,
                          axis_error_t *err) {
  axis_ASSERT(path, "Invalid argument.");
  axis_ASSERT(result, "Invalid argument.");

  if (!path || !strlen(path)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                    "path should not be empty.");
    }
    return false;
  }

  bool rc = true;

  axis_list_t split_by_colon = axis_LIST_INIT_VAL;
  axis_c_string_split(path, axis_STR_VALUE_PATH_OBJECT_DELIMITER,
                     &split_by_colon);

  axis_list_foreach (&split_by_colon, colon_iter) {
    axis_string_t *content_between_colon = axis_str_listnode_get(colon_iter.node);
    axis_ASSERT(content_between_colon, "Invalid argument.");

    if (!axis_value_path_parse_between_colon(content_between_colon, result)) {
      axis_list_clear(result);

      rc = false;
      goto done;
    }
  }

done:
  axis_list_clear(&split_by_colon);
  return rc;
}

axis_value_t *axis_value_peek_from_path(axis_value_t *base, const char *path,
                                      axis_error_t *err) {
  axis_ASSERT(base, "Invalid argument.");
  axis_ASSERT(path, "Invalid argument.");

  axis_value_t *result = NULL;

  axis_list_t path_items = axis_LIST_INIT_VAL;
  if (!axis_value_path_parse(path, &path_items, err)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                    "Failed to parse the path.");
    }
    goto done;
  }

  axis_list_foreach (&path_items, item_iter) {
    axis_value_path_item_t *item = axis_ptr_listnode_get(item_iter.node);
    axis_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case axis_VALUE_PATH_ITEM_TYPE_INVALID:
        axis_ASSERT(0, "Should not happen.");

        result = NULL;
        goto done;

      case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM:
        if (base->type != axis_TYPE_OBJECT) {
          if (err) {
            axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                          "Path is not corresponding to the value type.");
          }
          result = NULL;
          goto done;
        }

        axis_list_foreach (&base->content.object, object_iter) {
          axis_value_kv_t *kv = axis_ptr_listnode_get(object_iter.node);
          axis_ASSERT(kv && axis_value_kv_check_integrity(kv),
                     "Invalid argument.");

          if (axis_string_is_equal(&kv->key, &item->obj_item_str)) {
            if (item_iter.next) {
              base = kv->value;
            } else {
              result = kv->value;
            }
            break;
          }
        }
        break;

      case axis_VALUE_PATH_ITEM_TYPE_ARRAY_ITEM:
        if (base->type != axis_TYPE_ARRAY) {
          if (err) {
            axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                          "Path is not corresponding to the value type.");
          }
          result = NULL;
          goto done;
        }

        axis_list_foreach (&base->content.array, array_iter) {
          if (array_iter.index == item->arr_idx) {
            axis_value_t *array_item = axis_ptr_listnode_get(array_iter.node);
            axis_ASSERT(array_item, "Invalid argument.");

            if (item_iter.next) {
              base = array_item;
            } else {
              result = array_item;
            }
            break;
          }
        }
        break;

      default:
        axis_ASSERT(0, "Should not happen.");
        break;
    }
  }

done:
  axis_list_clear(&path_items);

  if (!result) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Failed to find property: %s",
                    path);
    }
  }

  return result;
}

bool axis_value_set_from_path_list_with_move(axis_value_t *base,
                                            axis_list_t *paths,
                                            axis_value_t *value,
                                            axis_UNUSED axis_error_t *err) {
  axis_ASSERT(base, "Invalid argument.");
  axis_ASSERT(paths, "Invalid argument.");
  axis_ASSERT(value, "Invalid argument.");

  bool result = true;

  axis_list_foreach (paths, item_iter) {
    axis_value_path_item_t *item = axis_ptr_listnode_get(item_iter.node);
    axis_ASSERT(item, "Invalid argument.");

    switch (item->type) {
      case axis_VALUE_PATH_ITEM_TYPE_INVALID:
        result = false;
        goto done;

      case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM: {
        if (base->type != axis_TYPE_OBJECT) {
          result = false;
          goto done;
        }

        bool found = false;

        axis_list_foreach (&base->content.object, object_iter) {
          axis_value_kv_t *kv = axis_ptr_listnode_get(object_iter.node);
          axis_ASSERT(kv && axis_value_kv_check_integrity(kv),
                     "Invalid argument.");

          if (axis_string_is_equal(&kv->key, &item->obj_item_str)) {
            found = true;

            if (item_iter.next == NULL) {
              // Override the original value.
              axis_value_destroy(kv->value);
              kv->value = value;
            } else {
              axis_value_path_item_t *next_item =
                  axis_ptr_listnode_get(item_iter.next);
              axis_ASSERT(next_item, "Invalid argument.");

              switch (next_item->type) {
                case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM:
                  if (!axis_value_is_object(kv->value)) {
                    axis_value_destroy(kv->value);
                    kv->value = axis_value_create_object_with_move(NULL);
                  }
                  break;
                case axis_VALUE_PATH_ITEM_TYPE_ARRAY_ITEM:
                  if (!axis_value_is_array(kv->value)) {
                    axis_value_destroy(kv->value);
                    kv->value = axis_value_create_array_with_move(NULL);
                  }
                  break;
                default:
                  axis_ASSERT(0, "Should not happen.");
                  break;
              }
            }

            base = kv->value;
            break;
          }
        }

        if (!found) {
          axis_value_t *new_base = NULL;

          if (item_iter.next == NULL) {
            // Override the original value.
            new_base = value;
          } else {
            axis_value_path_item_t *next_item =
                axis_ptr_listnode_get(item_iter.next);
            axis_ASSERT(next_item, "Invalid argument.");

            switch (next_item->type) {
              case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM:
                new_base = axis_value_create_object_with_move(NULL);
                break;
              case axis_VALUE_PATH_ITEM_TYPE_ARRAY_ITEM:
                new_base = axis_value_create_array_with_move(NULL);
                break;
              default:
                axis_ASSERT(0, "Should not happen.");
                break;
            }
          }

          axis_list_push_ptr_back(
              &base->content.object,
              axis_value_kv_create(axis_string_get_raw_str(&item->obj_item_str),
                                  new_base),
              (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);

          base = new_base;
        }
        break;
      }

      case axis_VALUE_PATH_ITEM_TYPE_ARRAY_ITEM: {
        if (base->type != axis_TYPE_ARRAY) {
          result = NULL;
          goto done;
        }

        bool found = false;

        axis_list_foreach (&base->content.array, array_iter) {
          if (array_iter.index == item->arr_idx) {
            found = true;

            axis_listnode_t *array_item_node = array_iter.node;
            axis_ASSERT(array_item_node, "Invalid argument.");

            axis_value_t *old_value = axis_ptr_listnode_get(array_item_node);
            if (item_iter.next == NULL) {
              // Override the original value.
              axis_value_destroy(old_value);

              axis_ptr_listnode_replace(
                  array_item_node, value,
                  (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
            } else {
              axis_value_path_item_t *next_item =
                  axis_ptr_listnode_get(item_iter.next);
              axis_ASSERT(next_item, "Invalid argument.");

              switch (next_item->type) {
                case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM:
                  if (!axis_value_is_object(old_value)) {
                    axis_value_destroy(old_value);

                    axis_ptr_listnode_replace(
                        array_item_node,
                        axis_value_create_object_with_move(NULL),
                        (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
                  }
                  break;
                case axis_VALUE_PATH_ITEM_TYPE_ARRAY_ITEM:
                  if (!axis_value_is_array(old_value)) {
                    axis_value_destroy(old_value);

                    axis_ptr_listnode_replace(
                        array_item_node, axis_value_create_array_with_move(NULL),
                        (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
                  }
                  break;
                default:
                  axis_ASSERT(0, "Should not happen.");
                  break;
              }
            }

            base = axis_ptr_listnode_get(array_item_node);
            break;
          }
        }

        if (!found) {
          for (size_t i = axis_list_size(&base->content.array);
               i < item->arr_idx; i++) {
            axis_list_push_ptr_back(
                &base->content.array, axis_value_create_invalid(),
                (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
          }

          axis_value_t *new_base = NULL;

          if (item_iter.next == NULL) {
            new_base = value;
          } else {
            axis_value_path_item_t *next_item =
                axis_ptr_listnode_get(item_iter.next);
            axis_ASSERT(next_item, "Invalid argument.");

            switch (next_item->type) {
              case axis_VALUE_PATH_ITEM_TYPE_OBJECT_ITEM:
                new_base = axis_value_create_object_with_move(NULL);
                break;
              case axis_VALUE_PATH_ITEM_TYPE_ARRAY_ITEM:
                new_base = axis_value_create_array_with_move(NULL);
                break;
              default:
                axis_ASSERT(0, "Should not happen.");
                break;
            }
          }

          axis_list_push_ptr_back(
              &base->content.array, new_base,
              (axis_ptr_listnode_destroy_func_t)axis_value_destroy);

          base = new_base;
        }
        break;
      }

      default:
        axis_ASSERT(0, "Should not happen.");
        break;
    }
  }

done:
  return result;
}

bool axis_value_set_from_path_str_with_move(axis_value_t *base, const char *path,
                                           axis_value_t *value,
                                           axis_error_t *err) {
  axis_ASSERT(base, "Invalid argument.");
  axis_ASSERT(path, "Invalid argument.");
  axis_ASSERT(value, "Invalid argument.");

  bool result = true;

  axis_list_t paths = axis_LIST_INIT_VAL;
  if (!axis_value_path_parse(path, &paths, err)) {
    goto done;
  }

  axis_value_set_from_path_list_with_move(base, &paths, value, err);

done:
  axis_list_clear(&paths);

  return result;
}
