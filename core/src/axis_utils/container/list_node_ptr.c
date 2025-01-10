//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"

static bool axis_ptr_listnode_check_integrity(axis_ptr_listnode_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_NORMAL_PTR_LISTNODE_SIGNATURE) {
    return false;
  }
  return axis_listnode_check_integrity(&self->hdr);
}

static void axis_ptr_listnode_destroy(axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_ptr_listnode_t *self = axis_listnode_to_ptr_listnode(self_);
  if (self->destroy) {
    self->destroy(self->ptr);
  }
}

axis_listnode_t *axis_ptr_listnode_create(
    void *ptr, axis_ptr_listnode_destroy_func_t destroy) {
  axis_ASSERT(ptr, "Invalid argument.");

  axis_ptr_listnode_t *self =
      (axis_ptr_listnode_t *)axis_malloc(sizeof(axis_ptr_listnode_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_listnode_init(&self->hdr, axis_ptr_listnode_destroy);
  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_NORMAL_PTR_LISTNODE_SIGNATURE);
  self->ptr = ptr;
  self->destroy = destroy;

  return (axis_listnode_t *)self;
}

axis_listnode_t *axis_listnode_from_ptr_listnode(axis_ptr_listnode_t *self) {
  axis_ASSERT(self && axis_ptr_listnode_check_integrity(self),
             "Invalid argument.");
  return &self->hdr;
}

axis_ptr_listnode_t *axis_listnode_to_ptr_listnode(axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_ptr_listnode_t *self = (axis_ptr_listnode_t *)self_;
  axis_ASSERT(axis_ptr_listnode_check_integrity(self), "Invalid argument.");
  return self;
}

void *axis_ptr_listnode_get(axis_listnode_t *self_) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_ptr_listnode_t *self = axis_listnode_to_ptr_listnode(self_);
  return self->ptr;
}

void axis_ptr_listnode_replace(axis_listnode_t *self_, void *ptr,
                              axis_ptr_listnode_destroy_func_t destroy) {
  axis_ASSERT(self_, "Invalid argument.");

  axis_ptr_listnode_t *self = axis_listnode_to_ptr_listnode(self_);
  self->ptr = ptr;
  self->destroy = destroy;
}
