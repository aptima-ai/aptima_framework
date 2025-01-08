//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/msg_not_connected_cnt.h"

#include "include_internal/axis_runtime/extension/extension.h"
#include "axis_utils/container/hash_table.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/memory.h"

#define axis_MSG_NOT_CONNECTED_COUNT_RESET_THRESHOLD (1000)

static axis_extension_output_msg_not_connected_count_t *
axis_extension_output_msg_not_connected_count_create(const char *msg_name) {
  axis_ASSERT(msg_name, "Invalid argument.");

  axis_extension_output_msg_not_connected_count_t *self =
      (axis_extension_output_msg_not_connected_count_t *)axis_MALLOC(
          sizeof(axis_extension_output_msg_not_connected_count_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_string_init_from_c_str(&self->msg_name, msg_name, strlen(msg_name));
  self->count = 0;

  return self;
}

static void axis_extension_output_msg_not_connected_count_destroy(
    axis_extension_output_msg_not_connected_count_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_deinit(&self->msg_name);

  axis_FREE(self);
}

static void axis_extension_output_msg_not_connected_count_hh_destroy(
    axis_hashhandle_t *hh) {
  axis_ASSERT(hh, "Should not happen.");

  axis_extension_output_msg_not_connected_count_t *entry =
      CONTAINER_OF_FROM_FIELD(
          hh, axis_extension_output_msg_not_connected_count_t, hh_in_map);
  axis_ASSERT(entry, "Should not happen.");

  axis_extension_output_msg_not_connected_count_destroy(entry);
}

bool axis_extension_increment_msg_not_connected_count(axis_extension_t *extension,
                                                     const char *msg_name) {
  axis_ASSERT(extension, "Invalid argument.");
  axis_ASSERT(msg_name, "Invalid argument.");

  axis_extension_output_msg_not_connected_count_t *entry = NULL;

  axis_hashhandle_t *hh = axis_hashtable_find_string(
      &extension->msg_not_connected_count_map, msg_name);
  if (!hh) {
    entry = axis_extension_output_msg_not_connected_count_create(msg_name);
    entry->count = 0;

    axis_hashtable_add_string(
        &extension->msg_not_connected_count_map, &entry->hh_in_map,
        axis_string_get_raw_str(&entry->msg_name),
        axis_extension_output_msg_not_connected_count_hh_destroy);
  } else {
    entry = CONTAINER_OF_FROM_FIELD(
        hh, axis_extension_output_msg_not_connected_count_t, hh_in_map);
    entry->count++;
  }

  if (entry->count % axis_MSG_NOT_CONNECTED_COUNT_RESET_THRESHOLD == 0) {
    entry->count = 0;  // Reset.
    return true;
  }
  return false;
}
