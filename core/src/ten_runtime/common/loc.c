//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/common/loc.h"

#include <stdlib.h>

#include "include_internal/ten_runtime/common/constant_str.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/lib/alloc.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_kv.h"
#include "ten_utils/value/value_object.h"

bool ten_loc_check_integrity(ten_loc_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_signature_get(&self->signature) != TEN_LOC_SIGNATURE) {
    return false;
  }

  if (!ten_string_is_empty(&self->extension_name)) {
    if (ten_string_is_empty(&self->extension_group_name)) {
      return false;
    }
  }

  return true;
}

ten_loc_t *ten_loc_create_empty(void) {
  ten_loc_t *self = (ten_loc_t *)TEN_MALLOC(sizeof(ten_loc_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  ten_loc_init_empty(self);

  return self;
}

ten_loc_t *ten_loc_create(const char *app_uri, const char *graph_id,
                          const char *extension_group_name,
                          const char *extension_name) {
  ten_loc_t *self = ten_loc_create_empty();

  ten_loc_set(self, app_uri, graph_id, extension_group_name, extension_name);
  TEN_ASSERT(ten_loc_check_integrity(self), "Should not happen.");

  return self;
}

ten_loc_t *ten_loc_create_from_value(ten_value_t *value) {
  TEN_ASSERT(value && ten_value_check_integrity(value), "Should not happen.");

  ten_loc_t *self = ten_loc_create_empty();

  ten_loc_set_from_value(self, value);
  TEN_ASSERT(ten_loc_check_integrity(self), "Should not happen.");

  return self;
}

ten_loc_t *ten_loc_clone(ten_loc_t *src) {
  TEN_ASSERT(src && ten_loc_check_integrity(src), "Should not happen.");

  ten_loc_t *self =
      ten_loc_create(ten_string_get_raw_str(&src->app_uri),
                     ten_string_get_raw_str(&src->graph_id),
                     ten_string_get_raw_str(&src->extension_group_name),
                     ten_string_get_raw_str(&src->extension_name));

  TEN_ASSERT(ten_loc_check_integrity(self), "Should not happen.");

  return self;
}

void ten_loc_copy(ten_loc_t *self, ten_loc_t *src) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(src && ten_loc_check_integrity(src), "Invalid argument.");

  ten_loc_set_from_loc(self, src);
}

void ten_loc_destroy(ten_loc_t *self) {
  TEN_ASSERT(self && ten_loc_check_integrity(self), "Should not happen.");

  ten_loc_deinit(self);
  TEN_FREE(self);
}

void ten_loc_init_empty(ten_loc_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_signature_set(&self->signature, TEN_LOC_SIGNATURE);

  ten_string_init(&self->app_uri);
  ten_string_init(&self->graph_id);
  ten_string_init(&self->extension_group_name);
  ten_string_init(&self->extension_name);
}

void ten_loc_init_from_loc(ten_loc_t *self, ten_loc_t *src) {
  TEN_ASSERT(self && src, "Should not happen.");

  ten_signature_set(&self->signature, TEN_LOC_SIGNATURE);

  ten_loc_init(self, ten_string_get_raw_str(&src->app_uri),
               ten_string_get_raw_str(&src->graph_id),
               ten_string_get_raw_str(&src->extension_group_name),
               ten_string_get_raw_str(&src->extension_name));

  TEN_ASSERT(ten_loc_check_integrity(self), "Should not happen.");
}

void ten_loc_set_from_loc(ten_loc_t *self, ten_loc_t *src) {
  TEN_ASSERT(self && ten_loc_check_integrity(self) && src,
             "Should not happen.");

  ten_loc_set(self, ten_string_get_raw_str(&src->app_uri),
              ten_string_get_raw_str(&src->graph_id),
              ten_string_get_raw_str(&src->extension_group_name),
              ten_string_get_raw_str(&src->extension_name));
}

void ten_loc_deinit(ten_loc_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_signature_set(&self->signature, 0);

  ten_string_deinit(&self->app_uri);
  ten_string_deinit(&self->graph_id);
  ten_string_deinit(&self->extension_group_name);
  ten_string_deinit(&self->extension_name);
}

void ten_loc_init(ten_loc_t *self, const char *app_uri, const char *graph_id,
                  const char *extension_group_name,
                  const char *extension_name) {
  TEN_ASSERT(self, "Should not happen.");

  ten_string_init_formatted(&self->app_uri, "%s", app_uri ? app_uri : "");
  ten_string_init_formatted(&self->graph_id, "%s", graph_id ? graph_id : "");
  ten_string_init_formatted(&self->extension_group_name, "%s",
                            extension_group_name ? extension_group_name : "");
  ten_string_init_formatted(&self->extension_name, "%s",
                            extension_name ? extension_name : "");

  TEN_ASSERT(ten_loc_check_integrity(self), "Should not happen.");
}

void ten_loc_set(ten_loc_t *self, const char *app_uri, const char *graph_id,
                 const char *extension_group_name, const char *extension_name) {
  TEN_ASSERT(self, "Should not happen.");

  ten_string_set_formatted(&self->app_uri, "%s", app_uri ? app_uri : "");
  ten_string_set_formatted(&self->graph_id, "%s", graph_id ? graph_id : "");
  ten_string_set_formatted(&self->extension_group_name, "%s",
                           extension_group_name ? extension_group_name : "");
  ten_string_set_formatted(&self->extension_name, "%s",
                           extension_name ? extension_name : "");

  TEN_ASSERT(ten_loc_check_integrity(self), "Should not happen.");
}

bool ten_loc_is_empty(ten_loc_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  if (ten_string_is_empty(&self->app_uri) &&
      ten_string_is_empty(&self->graph_id) &&
      ten_string_is_empty(&self->extension_group_name) &&
      ten_string_is_empty(&self->extension_name)) {
    return true;
  }
  return false;
}

void ten_loc_clear(ten_loc_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_string_clear(&self->app_uri);
  ten_string_clear(&self->graph_id);
  ten_string_clear(&self->extension_group_name);
  ten_string_clear(&self->extension_name);
}

bool ten_loc_is_equal(ten_loc_t *self, ten_loc_t *other) {
  TEN_ASSERT(self && other, "Should not happen.");

  return ten_string_is_equal(&self->app_uri, &other->app_uri) &&
         ten_string_is_equal(&self->graph_id, &other->graph_id) &&
         ten_string_is_equal(&self->extension_group_name,
                             &other->extension_group_name) &&
         ten_string_is_equal(&self->extension_name, &other->extension_name);
}

bool ten_loc_is_equal_with_value(ten_loc_t *self, const char *app_uri,
                                 const char *graph_id,
                                 const char *extension_group_name,
                                 const char *extension_name) {
  TEN_ASSERT(self && app_uri && extension_group_name && extension_name,
             "Should not happen.");

  return ten_string_is_equal_c_str(&self->app_uri, app_uri) &&
         ten_string_is_equal_c_str(&self->graph_id, graph_id) &&
         ten_string_is_equal_c_str(&self->extension_group_name,
                                   extension_group_name) &&
         ten_string_is_equal_c_str(&self->extension_name, extension_name);
}

void ten_loc_to_string(ten_loc_t *self, ten_string_t *result) {
  TEN_ASSERT(self && result && ten_loc_check_integrity(self),
             "Should not happen.");

  ten_string_set_formatted(result,
                           "app: %s, graph: %s, group: %s, extension: %s",
                           ten_string_get_raw_str(&self->app_uri),
                           ten_string_get_raw_str(&self->graph_id),
                           ten_string_get_raw_str(&self->extension_group_name),
                           ten_string_get_raw_str(&self->extension_name));
}

void ten_loc_to_json_string(ten_loc_t *self, ten_string_t *result) {
  TEN_ASSERT(self && result && ten_loc_check_integrity(self),
             "Should not happen.");

  ten_json_t *loc_json = ten_loc_to_json(self);
  TEN_ASSERT(loc_json, "Should not happen.");

  bool must_free = false;
  const char *loc_str = ten_json_to_string(loc_json, NULL, &must_free);
  TEN_ASSERT(loc_str, "Should not happen.");

  ten_string_init_formatted(result, loc_str);

  if (must_free) {
    TEN_FREE(loc_str);
  }
  ten_json_destroy(loc_json);
}

ten_json_t *ten_loc_to_json(ten_loc_t *self) {
  TEN_ASSERT(self && ten_loc_check_integrity(self), "Should not happen.");

  ten_json_t *loc_json = ten_json_create_object();
  TEN_ASSERT(loc_json, "Should not happen.");

  if (!ten_string_is_empty(&self->app_uri)) {
    ten_json_object_set_new(
        loc_json, TEN_STR_APP,
        ten_json_create_string(ten_string_get_raw_str(&self->app_uri)));
  }

  if (!ten_string_is_empty(&self->graph_id)) {
    ten_json_object_set_new(
        loc_json, TEN_STR_GRAPH,
        ten_json_create_string(ten_string_get_raw_str(&self->graph_id)));
  }

  if (!ten_string_is_empty(&self->extension_group_name)) {
    ten_json_object_set_new(loc_json, TEN_STR_EXTENSION_GROUP,
                            ten_json_create_string(ten_string_get_raw_str(
                                &self->extension_group_name)));
  }

  if (!ten_string_is_empty(&self->extension_name)) {
    ten_json_object_set_new(
        loc_json, TEN_STR_EXTENSION,
        ten_json_create_string(ten_string_get_raw_str(&self->extension_name)));
  }

  return loc_json;
}

static bool ten_loc_set_value(ten_loc_t *self, ten_value_t *value) {
  TEN_ASSERT(self && ten_loc_check_integrity(self), "Should not happen.");
  TEN_ASSERT(value && ten_value_check_integrity(value), "Should not happen.");

  ten_list_t loc_fields = TEN_LIST_INIT_VAL;

  if (!ten_string_is_empty(&self->app_uri)) {
    ten_list_push_ptr_back(
        &loc_fields,
        ten_value_kv_create(
            TEN_STR_APP,
            ten_value_create_string(ten_string_get_raw_str(&self->app_uri))),
        (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);
  }

  if (!ten_string_is_empty(&self->graph_id)) {
    ten_list_push_ptr_back(
        &loc_fields,
        ten_value_kv_create(
            TEN_STR_GRAPH,
            ten_value_create_string(ten_string_get_raw_str(&self->graph_id))),
        (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);
  }

  if (!ten_string_is_empty(&self->extension_group_name)) {
    ten_list_push_ptr_back(
        &loc_fields,
        ten_value_kv_create(TEN_STR_EXTENSION_GROUP,
                            ten_value_create_string(ten_string_get_raw_str(
                                &self->extension_group_name))),
        (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);
  }

  if (!ten_string_is_empty(&self->extension_name)) {
    ten_list_push_ptr_back(
        &loc_fields,
        ten_value_kv_create(TEN_STR_EXTENSION,
                            ten_value_create_string(
                                ten_string_get_raw_str(&self->extension_name))),
        (ten_ptr_listnode_destroy_func_t)ten_value_kv_destroy);
  }

  bool rc = ten_value_init_object_with_move(value, &loc_fields);

  ten_list_clear(&loc_fields);
  return rc;
}

ten_value_t *ten_loc_to_value(ten_loc_t *self) {
  TEN_ASSERT(self && ten_loc_check_integrity(self), "Should not happen.");

  ten_value_t *loc_value = ten_value_create_object_with_move(NULL);
  TEN_ASSERT(loc_value, "Should not happen.");

  if (ten_loc_set_value(self, loc_value)) {
    return loc_value;
  } else {
    ten_value_destroy(loc_value);
    return NULL;
  }
}

void ten_loc_set_from_value(ten_loc_t *self, ten_value_t *value) {
  TEN_ASSERT(self && value, "Should not happen.");

  ten_value_t *app_value = ten_value_object_peek(value, TEN_STR_APP);
  ten_value_t *graph_value = ten_value_object_peek(value, TEN_STR_GRAPH);
  ten_value_t *extension_group_value =
      ten_value_object_peek(value, TEN_STR_EXTENSION_GROUP);
  ten_value_t *extension_value =
      ten_value_object_peek(value, TEN_STR_EXTENSION);

  if (app_value) {
    TEN_ASSERT(ten_value_is_string(app_value), "Should not happen.");

    const char *app_str = ten_value_peek_raw_str(app_value, NULL);
    if (app_str && strlen(app_str) > 0) {
      ten_string_set_from_c_str(&self->app_uri, app_str, strlen(app_str));
    }
  }

  if (graph_value) {
    TEN_ASSERT(ten_value_is_string(graph_value), "Should not happen.");

    const char *graph_str = ten_value_peek_raw_str(graph_value, NULL);
    if (graph_str && strlen(graph_str) > 0) {
      ten_string_set_from_c_str(&self->graph_id, graph_str, strlen(graph_str));
    }
  }

  if (extension_group_value) {
    TEN_ASSERT(ten_value_is_string(extension_group_value),
               "Should not happen.");

    const char *group_name_str =
        ten_value_peek_raw_str(extension_group_value, NULL);
    if (group_name_str && strlen(group_name_str) > 0) {
      ten_string_set_from_c_str(&self->extension_group_name, group_name_str,
                                strlen(group_name_str));
    }
  }

  if (extension_value) {
    TEN_ASSERT(ten_value_is_string(extension_value), "Should not happen.");

    const char *extension_name_str =
        ten_value_peek_raw_str(extension_value, NULL);
    if (extension_name_str && strlen(extension_name_str) > 0) {
      ten_string_set_from_c_str(&self->extension_name, extension_name_str,
                                strlen(extension_name_str));
    }
  }
}

void ten_loc_init_from_value(ten_loc_t *self, ten_value_t *value) {
  TEN_ASSERT(self && value, "Should not happen.");

  ten_loc_init_empty(self);
  ten_loc_set_from_value(self, value);
}
