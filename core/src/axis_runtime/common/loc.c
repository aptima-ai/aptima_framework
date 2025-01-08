//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/common/loc.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/common/constant_str.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_ptr.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_kv.h"
#include "axis_utils/value/value_object.h"

bool axis_loc_check_integrity(axis_loc_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_LOC_SIGNATURE) {
    return false;
  }

  if (!axis_string_is_empty(&self->extension_name)) {
    if (axis_string_is_empty(&self->extension_group_name)) {
      return false;
    }
  }

  return true;
}

axis_loc_t *axis_loc_create_empty(void) {
  axis_loc_t *self = (axis_loc_t *)axis_MALLOC(sizeof(axis_loc_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_loc_init_empty(self);

  return self;
}

axis_loc_t *axis_loc_create(const char *app_uri, const char *graph_id,
                          const char *extension_group_name,
                          const char *extension_name) {
  axis_loc_t *self = axis_loc_create_empty();

  axis_loc_set(self, app_uri, graph_id, extension_group_name, extension_name);
  axis_ASSERT(axis_loc_check_integrity(self), "Should not happen.");

  return self;
}

axis_loc_t *axis_loc_create_from_value(axis_value_t *value) {
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  axis_loc_t *self = axis_loc_create_empty();

  axis_loc_set_from_value(self, value);
  axis_ASSERT(axis_loc_check_integrity(self), "Should not happen.");

  return self;
}

axis_loc_t *axis_loc_clone(axis_loc_t *src) {
  axis_ASSERT(src && axis_loc_check_integrity(src), "Should not happen.");

  axis_loc_t *self =
      axis_loc_create(axis_string_get_raw_str(&src->app_uri),
                     axis_string_get_raw_str(&src->graph_id),
                     axis_string_get_raw_str(&src->extension_group_name),
                     axis_string_get_raw_str(&src->extension_name));

  axis_ASSERT(axis_loc_check_integrity(self), "Should not happen.");

  return self;
}

void axis_loc_copy(axis_loc_t *self, axis_loc_t *src) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(src && axis_loc_check_integrity(src), "Invalid argument.");

  axis_loc_set_from_loc(self, src);
}

void axis_loc_destroy(axis_loc_t *self) {
  axis_ASSERT(self && axis_loc_check_integrity(self), "Should not happen.");

  axis_loc_deinit(self);
  axis_FREE(self);
}

void axis_loc_init_empty(axis_loc_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_signature_set(&self->signature, axis_LOC_SIGNATURE);

  axis_string_init(&self->app_uri);
  axis_string_init(&self->graph_id);
  axis_string_init(&self->extension_group_name);
  axis_string_init(&self->extension_name);
}

void axis_loc_init_from_loc(axis_loc_t *self, axis_loc_t *src) {
  axis_ASSERT(self && src, "Should not happen.");

  axis_signature_set(&self->signature, axis_LOC_SIGNATURE);

  axis_loc_init(self, axis_string_get_raw_str(&src->app_uri),
               axis_string_get_raw_str(&src->graph_id),
               axis_string_get_raw_str(&src->extension_group_name),
               axis_string_get_raw_str(&src->extension_name));

  axis_ASSERT(axis_loc_check_integrity(self), "Should not happen.");
}

void axis_loc_set_from_loc(axis_loc_t *self, axis_loc_t *src) {
  axis_ASSERT(self && axis_loc_check_integrity(self) && src,
             "Should not happen.");

  axis_loc_set(self, axis_string_get_raw_str(&src->app_uri),
              axis_string_get_raw_str(&src->graph_id),
              axis_string_get_raw_str(&src->extension_group_name),
              axis_string_get_raw_str(&src->extension_name));
}

void axis_loc_deinit(axis_loc_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_signature_set(&self->signature, 0);

  axis_string_deinit(&self->app_uri);
  axis_string_deinit(&self->graph_id);
  axis_string_deinit(&self->extension_group_name);
  axis_string_deinit(&self->extension_name);
}

void axis_loc_init(axis_loc_t *self, const char *app_uri, const char *graph_id,
                  const char *extension_group_name,
                  const char *extension_name) {
  axis_ASSERT(self, "Should not happen.");

  axis_string_init_formatted(&self->app_uri, "%s", app_uri ? app_uri : "");
  axis_string_init_formatted(&self->graph_id, "%s", graph_id ? graph_id : "");
  axis_string_init_formatted(&self->extension_group_name, "%s",
                            extension_group_name ? extension_group_name : "");
  axis_string_init_formatted(&self->extension_name, "%s",
                            extension_name ? extension_name : "");

  axis_ASSERT(axis_loc_check_integrity(self), "Should not happen.");
}

void axis_loc_set(axis_loc_t *self, const char *app_uri, const char *graph_id,
                 const char *extension_group_name, const char *extension_name) {
  axis_ASSERT(self, "Should not happen.");

  axis_string_set_formatted(&self->app_uri, "%s", app_uri ? app_uri : "");
  axis_string_set_formatted(&self->graph_id, "%s", graph_id ? graph_id : "");
  axis_string_set_formatted(&self->extension_group_name, "%s",
                           extension_group_name ? extension_group_name : "");
  axis_string_set_formatted(&self->extension_name, "%s",
                           extension_name ? extension_name : "");

  axis_ASSERT(axis_loc_check_integrity(self), "Should not happen.");
}

bool axis_loc_is_empty(axis_loc_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_string_is_empty(&self->app_uri) &&
      axis_string_is_empty(&self->graph_id) &&
      axis_string_is_empty(&self->extension_group_name) &&
      axis_string_is_empty(&self->extension_name)) {
    return true;
  }
  return false;
}

void axis_loc_clear(axis_loc_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_string_clear(&self->app_uri);
  axis_string_clear(&self->graph_id);
  axis_string_clear(&self->extension_group_name);
  axis_string_clear(&self->extension_name);
}

bool axis_loc_is_equal(axis_loc_t *self, axis_loc_t *other) {
  axis_ASSERT(self && other, "Should not happen.");

  return axis_string_is_equal(&self->app_uri, &other->app_uri) &&
         axis_string_is_equal(&self->graph_id, &other->graph_id) &&
         axis_string_is_equal(&self->extension_group_name,
                             &other->extension_group_name) &&
         axis_string_is_equal(&self->extension_name, &other->extension_name);
}

bool axis_loc_is_equal_with_value(axis_loc_t *self, const char *app_uri,
                                 const char *graph_id,
                                 const char *extension_group_name,
                                 const char *extension_name) {
  axis_ASSERT(self && app_uri && extension_group_name && extension_name,
             "Should not happen.");

  return axis_string_is_equal_c_str(&self->app_uri, app_uri) &&
         axis_string_is_equal_c_str(&self->graph_id, graph_id) &&
         axis_string_is_equal_c_str(&self->extension_group_name,
                                   extension_group_name) &&
         axis_string_is_equal_c_str(&self->extension_name, extension_name);
}

void axis_loc_to_string(axis_loc_t *self, axis_string_t *result) {
  axis_ASSERT(self && result && axis_loc_check_integrity(self),
             "Should not happen.");

  axis_string_set_formatted(result,
                           "app: %s, graph: %s, group: %s, extension: %s",
                           axis_string_get_raw_str(&self->app_uri),
                           axis_string_get_raw_str(&self->graph_id),
                           axis_string_get_raw_str(&self->extension_group_name),
                           axis_string_get_raw_str(&self->extension_name));
}

void axis_loc_to_json_string(axis_loc_t *self, axis_string_t *result) {
  axis_ASSERT(self && result && axis_loc_check_integrity(self),
             "Should not happen.");

  axis_json_t *loc_json = axis_loc_to_json(self);
  axis_ASSERT(loc_json, "Should not happen.");

  bool must_free = false;
  const char *loc_str = axis_json_to_string(loc_json, NULL, &must_free);
  axis_ASSERT(loc_str, "Should not happen.");

  axis_string_init_formatted(result, loc_str);

  if (must_free) {
    axis_FREE(loc_str);
  }
  axis_json_destroy(loc_json);
}

axis_json_t *axis_loc_to_json(axis_loc_t *self) {
  axis_ASSERT(self && axis_loc_check_integrity(self), "Should not happen.");

  axis_json_t *loc_json = axis_json_create_object();
  axis_ASSERT(loc_json, "Should not happen.");

  if (!axis_string_is_empty(&self->app_uri)) {
    axis_json_object_set_new(
        loc_json, axis_STR_APP,
        axis_json_create_string(axis_string_get_raw_str(&self->app_uri)));
  }

  if (!axis_string_is_empty(&self->graph_id)) {
    axis_json_object_set_new(
        loc_json, axis_STR_GRAPH,
        axis_json_create_string(axis_string_get_raw_str(&self->graph_id)));
  }

  if (!axis_string_is_empty(&self->extension_group_name)) {
    axis_json_object_set_new(loc_json, axis_STR_EXTENSION_GROUP,
                            axis_json_create_string(axis_string_get_raw_str(
                                &self->extension_group_name)));
  }

  if (!axis_string_is_empty(&self->extension_name)) {
    axis_json_object_set_new(
        loc_json, axis_STR_EXTENSION,
        axis_json_create_string(axis_string_get_raw_str(&self->extension_name)));
  }

  return loc_json;
}

static bool axis_loc_set_value(axis_loc_t *self, axis_value_t *value) {
  axis_ASSERT(self && axis_loc_check_integrity(self), "Should not happen.");
  axis_ASSERT(value && axis_value_check_integrity(value), "Should not happen.");

  axis_list_t loc_fields = axis_LIST_INIT_VAL;

  if (!axis_string_is_empty(&self->app_uri)) {
    axis_list_push_ptr_back(
        &loc_fields,
        axis_value_kv_create(
            axis_STR_APP,
            axis_value_create_string(axis_string_get_raw_str(&self->app_uri))),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  if (!axis_string_is_empty(&self->graph_id)) {
    axis_list_push_ptr_back(
        &loc_fields,
        axis_value_kv_create(
            axis_STR_GRAPH,
            axis_value_create_string(axis_string_get_raw_str(&self->graph_id))),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  if (!axis_string_is_empty(&self->extension_group_name)) {
    axis_list_push_ptr_back(
        &loc_fields,
        axis_value_kv_create(axis_STR_EXTENSION_GROUP,
                            axis_value_create_string(axis_string_get_raw_str(
                                &self->extension_group_name))),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  if (!axis_string_is_empty(&self->extension_name)) {
    axis_list_push_ptr_back(
        &loc_fields,
        axis_value_kv_create(axis_STR_EXTENSION,
                            axis_value_create_string(
                                axis_string_get_raw_str(&self->extension_name))),
        (axis_ptr_listnode_destroy_func_t)axis_value_kv_destroy);
  }

  bool rc = axis_value_init_object_with_move(value, &loc_fields);

  axis_list_clear(&loc_fields);
  return rc;
}

axis_value_t *axis_loc_to_value(axis_loc_t *self) {
  axis_ASSERT(self && axis_loc_check_integrity(self), "Should not happen.");

  axis_value_t *loc_value = axis_value_create_object_with_move(NULL);
  axis_ASSERT(loc_value, "Should not happen.");

  if (axis_loc_set_value(self, loc_value)) {
    return loc_value;
  } else {
    axis_value_destroy(loc_value);
    return NULL;
  }
}

void axis_loc_set_from_value(axis_loc_t *self, axis_value_t *value) {
  axis_ASSERT(self && value, "Should not happen.");

  axis_value_t *app_value = axis_value_object_peek(value, axis_STR_APP);
  axis_value_t *graph_value = axis_value_object_peek(value, axis_STR_GRAPH);
  axis_value_t *extension_group_value =
      axis_value_object_peek(value, axis_STR_EXTENSION_GROUP);
  axis_value_t *extension_value =
      axis_value_object_peek(value, axis_STR_EXTENSION);

  if (app_value) {
    axis_ASSERT(axis_value_is_string(app_value), "Should not happen.");

    const char *app_str = axis_value_peek_raw_str(app_value, NULL);
    if (app_str && strlen(app_str) > 0) {
      axis_string_set_from_c_str(&self->app_uri, app_str, strlen(app_str));
    }
  }

  if (graph_value) {
    axis_ASSERT(axis_value_is_string(graph_value), "Should not happen.");

    const char *graph_str = axis_value_peek_raw_str(graph_value, NULL);
    if (graph_str && strlen(graph_str) > 0) {
      axis_string_set_from_c_str(&self->graph_id, graph_str, strlen(graph_str));
    }
  }

  if (extension_group_value) {
    axis_ASSERT(axis_value_is_string(extension_group_value),
               "Should not happen.");

    const char *group_name_str =
        axis_value_peek_raw_str(extension_group_value, NULL);
    if (group_name_str && strlen(group_name_str) > 0) {
      axis_string_set_from_c_str(&self->extension_group_name, group_name_str,
                                strlen(group_name_str));
    }
  }

  if (extension_value) {
    axis_ASSERT(axis_value_is_string(extension_value), "Should not happen.");

    const char *extension_name_str =
        axis_value_peek_raw_str(extension_value, NULL);
    if (extension_name_str && strlen(extension_name_str) > 0) {
      axis_string_set_from_c_str(&self->extension_name, extension_name_str,
                                strlen(extension_name_str));
    }
  }
}

void axis_loc_init_from_value(axis_loc_t *self, axis_value_t *value) {
  axis_ASSERT(self && value, "Should not happen.");

  axis_loc_init_empty(self);
  axis_loc_set_from_value(self, value);
}
