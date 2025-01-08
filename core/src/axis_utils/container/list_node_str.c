//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>
#include <string.h>

#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"

static bool axis_str_listnode_check_integrity(axis_str_listnode_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_STR_LISTNODE_SIGNATURE) {
    return false;
  }
  return axis_listnode_check_integrity(&self->hdr);
}

static void axis_str_listnode_destroy(axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_str_listnode_t *self = axis_listnode_to_str_listnode(self_);
  axis_string_deinit(&self->str);
}

axis_listnode_t *axis_str_listnode_create(const char *str) {
  axis_ASSERT(str, "Invalid argument.");

  return axis_str_listnode_create_with_size(str, strlen(str));
}

axis_listnode_t *axis_str_listnode_create_with_size(const char *str,
                                                  size_t size) {
  axis_ASSERT(str, "Invalid argument.");

  axis_str_listnode_t *self =
      (axis_str_listnode_t *)axis_malloc(sizeof(axis_str_listnode_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_listnode_init(&self->hdr, axis_str_listnode_destroy);
  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_STR_LISTNODE_SIGNATURE);

  axis_string_init_formatted(&self->str, "%.*s", size, str);

  return (axis_listnode_t *)self;
}

axis_listnode_t *axis_listnode_from_str_listnode(axis_str_listnode_t *self) {
  axis_ASSERT(self && axis_str_listnode_check_integrity(self),
             "Invalid argument.");
  return &self->hdr;
}

axis_str_listnode_t *axis_listnode_to_str_listnode(axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_str_listnode_t *self = (axis_str_listnode_t *)self_;
  axis_ASSERT(axis_str_listnode_check_integrity(self), "Invalid argument.");
  return self;
}

axis_string_t *axis_str_listnode_get(axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_str_listnode_t *self = axis_listnode_to_str_listnode(self_);
  return &self->str;
}
