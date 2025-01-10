//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <stdlib.h>
#include <string.h>

#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"

static bool axis_smart_ptr_listnode_check_integrity(
    axis_smart_ptr_listnode_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_SMART_PTR_LISTNODE_SIGNATURE) {
    return false;
  }
  return axis_listnode_check_integrity(&self->hdr);
}

static void axis_smart_ptr_listnode_destroy(axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_smart_ptr_listnode_t *self = axis_listnode_to_smart_ptr_listnode(self_);
  axis_smart_ptr_destroy(self->ptr);
  self->ptr = NULL;
}

axis_listnode_t *axis_smart_ptr_listnode_create(axis_smart_ptr_t *ptr) {
  axis_ASSERT(ptr, "Invalid argument.");

  axis_smart_ptr_listnode_t *self =
      (axis_smart_ptr_listnode_t *)axis_malloc(sizeof(axis_smart_ptr_listnode_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_listnode_init(&self->hdr, axis_smart_ptr_listnode_destroy);
  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_SMART_PTR_LISTNODE_SIGNATURE);
  self->ptr = axis_smart_ptr_clone(ptr);

  return (axis_listnode_t *)self;
}

axis_smart_ptr_listnode_t *axis_listnode_to_smart_ptr_listnode(
    axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_smart_ptr_listnode_t *self = (axis_smart_ptr_listnode_t *)self_;
  axis_ASSERT(axis_smart_ptr_listnode_check_integrity(self), "Invalid argument.");

  return self;
}

axis_listnode_t *axis_listnode_from_smart_ptr_listnode(
    axis_smart_ptr_listnode_t *self) {
  axis_ASSERT(self && axis_smart_ptr_listnode_check_integrity(self),
             "Invalid argument.");
  return &self->hdr;
}

axis_smart_ptr_t *axis_smart_ptr_listnode_get(axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_smart_ptr_listnode_t *self = axis_listnode_to_smart_ptr_listnode(self_);
  return self->ptr;
}
