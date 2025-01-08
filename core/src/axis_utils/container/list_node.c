//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/container/list_node.h"

#include <stdlib.h>
#include <string.h>

#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"

bool axis_listnode_check_integrity(axis_listnode_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_LISTNODE_SIGNATURE) {
    return false;
  }
  return true;
}

void axis_listnode_init(axis_listnode_t *self, void *destroy) {
  axis_ASSERT(self, "Invalid argument.");

  axis_signature_set(&self->signature, (axis_signature_t)axis_LISTNODE_SIGNATURE);
  self->destroy = destroy;
  self->next = NULL;
  self->prev = NULL;
}

void axis_listnode_destroy(axis_listnode_t *self) {
  axis_ASSERT(self && axis_listnode_check_integrity(self), "Invalid argument.");

  if (self->destroy) {
    self->destroy(self);
  }
  axis_free(self);
}

void axis_listnode_destroy_only(axis_listnode_t *self) {
  axis_ASSERT(self && axis_listnode_check_integrity(self), "Invalid argument.");
  axis_free(self);
}
