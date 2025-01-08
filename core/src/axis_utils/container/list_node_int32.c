//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"

static bool axis_int32_listnode_check_integrity(axis_int32_listnode_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_INT32_LISTNODE_SIGNATURE) {
    return false;
  }
  return axis_listnode_check_integrity(&self->hdr);
}

static void axis_int32_listnode_destroy(axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");
}

axis_listnode_t *axis_int32_listnode_create(int32_t int32) {
  axis_int32_listnode_t *self =
      (axis_int32_listnode_t *)axis_malloc(sizeof(axis_int32_listnode_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_listnode_init(&self->hdr, axis_int32_listnode_destroy);
  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_INT32_LISTNODE_SIGNATURE);
  self->int32 = int32;

  return (axis_listnode_t *)self;
}

axis_listnode_t *axis_listnode_from_int32_listnode(axis_int32_listnode_t *self) {
  axis_ASSERT(self && axis_int32_listnode_check_integrity(self),
             "Invalid argument.");
  return &self->hdr;
}

axis_int32_listnode_t *axis_listnode_to_int32_listnode(axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_int32_listnode_t *self = (axis_int32_listnode_t *)self_;
  axis_ASSERT(axis_int32_listnode_check_integrity(self), "Invalid argument.");
  return self;
}

int32_t axis_int32_listnode_get(axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_int32_listnode_t *self = axis_listnode_to_int32_listnode(self_);
  return self->int32;
}
