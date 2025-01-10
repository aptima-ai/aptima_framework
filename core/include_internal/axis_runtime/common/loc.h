//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/json.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/value/value.h"

#define axis_LOC_SIGNATURE 0x581B639EF70CBC5DU

typedef struct axis_extension_t axis_extension_t;

// This type represents the dynamic information of a extension. Do not mix
// static information of a extension here.
//
// - dynamic information
//   Something like the 'object' information in an object-oriented programming
//   language, which is how to 'locate' the object instance.
//   Therefore, the dynamic information of a extension is the information
//   relevant to the location of a extension instance. Ex: the uri of the app,
//   the graph_id of the engine, the name of the extension group and the
//   extension.
//
// - static information
//   Something like the 'type' information in an object-oriented programming
//   language, which is how to 'create' the object instance.
//   Therefore, the static information of a extension is the information
//   relevant to the creating logic of a extension instance. Ex: the addon
//   name of the extension group and the extension.
typedef struct axis_loc_t {
  axis_signature_t signature;

  axis_string_t app_uri;
  axis_string_t graph_id;
  axis_string_t extension_group_name;
  axis_string_t extension_name;
} axis_loc_t;

axis_RUNTIME_PRIVATE_API bool axis_loc_check_integrity(axis_loc_t *self);

axis_RUNTIME_PRIVATE_API axis_loc_t *axis_loc_create_empty(void);

axis_RUNTIME_API axis_loc_t *axis_loc_create(const char *app_uri,
                                          const char *graph_id,
                                          const char *extension_group_name,
                                          const char *extension_name);

axis_RUNTIME_PRIVATE_API axis_loc_t *axis_loc_create_from_value(
    axis_value_t *value);

axis_RUNTIME_API void axis_loc_destroy(axis_loc_t *self);

axis_RUNTIME_PRIVATE_API axis_loc_t *axis_loc_clone(axis_loc_t *src);

axis_RUNTIME_PRIVATE_API void axis_loc_copy(axis_loc_t *self, axis_loc_t *src);

axis_RUNTIME_PRIVATE_API void axis_loc_init_empty(axis_loc_t *self);

axis_RUNTIME_PRIVATE_API void axis_loc_init(axis_loc_t *self, const char *app_uri,
                                          const char *graph_id,
                                          const char *extension_group_name,
                                          const char *extension_name);

axis_RUNTIME_PRIVATE_API void axis_loc_init_from_loc(axis_loc_t *self,
                                                   axis_loc_t *src);

axis_RUNTIME_PRIVATE_API void axis_loc_init_from_value(axis_loc_t *self,
                                                     axis_value_t *value);

axis_RUNTIME_PRIVATE_API void axis_loc_deinit(axis_loc_t *self);

axis_RUNTIME_PRIVATE_API void axis_loc_set(axis_loc_t *self, const char *app_uri,
                                         const char *graph_id,
                                         const char *extension_group_name,
                                         const char *extension_name);

axis_RUNTIME_PRIVATE_API void axis_loc_set_from_loc(axis_loc_t *self,
                                                  axis_loc_t *src);

axis_RUNTIME_PRIVATE_API void axis_loc_set_from_value(axis_loc_t *self,
                                                    axis_value_t *value);

axis_RUNTIME_PRIVATE_API bool axis_loc_is_empty(axis_loc_t *self);

axis_RUNTIME_PRIVATE_API void axis_loc_clear(axis_loc_t *self);

axis_RUNTIME_PRIVATE_API bool axis_loc_is_equal(axis_loc_t *self,
                                              axis_loc_t *other);

axis_RUNTIME_PRIVATE_API bool axis_loc_is_equal_with_value(
    axis_loc_t *self, const char *app_uri, const char *graph_id,
    const char *extension_group_name, const char *extension_name);

axis_RUNTIME_PRIVATE_API void axis_loc_to_string(axis_loc_t *self,
                                               axis_string_t *result);

axis_RUNTIME_PRIVATE_API axis_json_t *axis_loc_to_json(axis_loc_t *self);

axis_RUNTIME_PRIVATE_API void axis_loc_to_json_string(axis_loc_t *self,
                                                    axis_string_t *result);

axis_RUNTIME_PRIVATE_API axis_value_t *axis_loc_to_value(axis_loc_t *self);
