//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/container/list.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

bool axis_list_check_integrity(axis_list_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_LIST_SIGNATURE) {
    return false;
  }

  if (!self->size) {
    if (self->front != NULL || self->back != NULL) {
      // Conflicting between 'size' and actual 'data'.
      return false;
    }
  } else {
    if (self->front == NULL || self->back == NULL) {
      return false;
    }

    if (self->size == 1) {
      if (self->front != self->back) {
        return false;
      }
    } else {
      if (self->front == self->back) {
        return false;
      }
    }

    if (self->front->prev != NULL) {
      return false;
    }
    if (self->back->next != NULL) {
      return false;
    }
  }
  return true;
}

axis_list_t *axis_list_create(void) {
  axis_list_t *self = (axis_list_t *)axis_MALLOC(sizeof(axis_list_t));
  axis_ASSERT(self, "Failed to allocate memory.");
  axis_list_init(self);
  return self;
}

void axis_list_destroy(axis_list_t *self) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");
  axis_list_clear(self);
  axis_FREE(self);
}

void axis_list_init(axis_list_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_signature_set(&self->signature, (axis_signature_t)axis_LIST_SIGNATURE);
  self->size = 0;
  self->front = NULL;
  self->back = NULL;
}

void axis_list_clear(axis_list_t *self) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");

  axis_list_foreach (self, iter) {
    axis_listnode_destroy(iter.node);
  }

  self->size = 0;
  self->front = NULL;
  self->back = NULL;
}

void axis_list_reset(axis_list_t *self) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");

  self->size = 0;
  self->front = NULL;
  self->back = NULL;
}

bool axis_list_is_empty(axis_list_t *self) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");
  return !axis_list_size(self);
}

size_t axis_list_size(axis_list_t *self) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");
  return self->size;
}

void axis_list_swap(axis_list_t *self, axis_list_t *target) {
  axis_ASSERT(self && target && axis_list_check_integrity(self) &&
                 axis_list_check_integrity(target),
             "Invalid argument.");

  axis_listnode_t *self_front = self->front;
  axis_listnode_t *self_back = self->back;
  size_t self_size = self->size;

  self->front = target->front;
  self->back = target->back;
  self->size = target->size;

  target->front = self_front;
  target->back = self_back;
  target->size = self_size;
}

void axis_list_concat(axis_list_t *self, axis_list_t *target) {
  axis_ASSERT(self && target && axis_list_check_integrity(self) &&
                 axis_list_check_integrity(target),
             "Invalid argument.");

  if (axis_list_size(target) == 0) {
    return;
  }

  if (axis_list_size(self) == 0) {
    axis_list_swap(self, target);
    return;
  }

  self->back->next = target->front;
  target->front->prev = self->back;
  self->back = target->back;
  self->size += target->size;

  target->front = NULL;
  target->back = NULL;
  target->size = 0;
}

void axis_list_detach_node(axis_list_t *self, axis_listnode_t *curr_node) {
  if (!self || !axis_list_check_integrity(self)) {
    axis_ASSERT(0, "Invalid argument.");
    return;
  }

  if (axis_list_is_empty(self)) {
    axis_ASSERT(0, "Invalid argument.");
    return;
  }

  if (!curr_node) {
    axis_ASSERT(0, "Invalid argument.");
    return;
  }

  if (axis_list_size(self) == 1) {
    axis_ASSERT(self->front == curr_node, "Invalid list.");

    self->front = NULL;
    self->back = NULL;
  } else {
    if (curr_node == self->front) {
      axis_ASSERT(curr_node->prev == NULL && curr_node->next->prev == curr_node,
                 "Invalid list.");

      self->front->next->prev = NULL;
      self->front = self->front->next;
    } else if (curr_node == self->back) {
      axis_ASSERT(curr_node->next == NULL && curr_node->prev->next == curr_node,
                 "Invalid list.");

      self->back->prev->next = NULL;
      self->back = self->back->prev;
    } else {
      axis_ASSERT(curr_node->prev->next == curr_node &&
                     curr_node->next->prev == curr_node,
                 "Invalid list.");

      curr_node->prev->next = curr_node->next;
      curr_node->next->prev = curr_node->prev;
    }
  }

  --self->size;
}

void axis_list_remove_node(axis_list_t *self, axis_listnode_t *node) {
  axis_ASSERT(self && axis_list_check_integrity(self) && node &&
                 !axis_list_is_empty(self),
             "Invalid argument.");

  axis_list_detach_node(self, node);
  axis_listnode_destroy(node);
}

axis_listnode_t *axis_list_front(axis_list_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_list_check_integrity(self), "Invalid argument.");
  return self->front;
}

axis_listnode_t *axis_list_back(axis_list_t *self) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");
  return self->back;
}

void axis_list_push_front(axis_list_t *self, axis_listnode_t *node) {
  axis_ASSERT(self && axis_list_check_integrity(self) && node,
             "Invalid argument.");

  if (axis_list_is_empty(self)) {
    self->back = self->front = node;
    node->prev = node->next = NULL;
  } else {
    node->next = self->front;
    node->prev = NULL;

    self->front->prev = node;
    self->front = node;
  }
  ++self->size;
}

void axis_list_push_back(axis_list_t *self, axis_listnode_t *node) {
  axis_ASSERT(self && axis_list_check_integrity(self) && node,
             "Invalid argument.");

  if (axis_list_is_empty(self)) {
    self->front = self->back = node;
    node->next = node->prev = NULL;
  } else {
    node->next = NULL;
    node->prev = self->back;

    self->back->next = node;
    self->back = node;
  }
  ++self->size;
}

axis_listnode_t *axis_list_pop_front(axis_list_t *self) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");

  if (axis_list_is_empty(self)) {
    return NULL;
  }
  axis_listnode_t *node = self->front;
  if (axis_list_size(self) == 1) {
    self->back = self->front = NULL;
    node->prev = node->next = NULL;
  } else {
    self->front = self->front->next;
    self->front->prev = NULL;

    node->next = NULL;
  }
  --self->size;

  return node;
}

void axis_list_remove_front(axis_list_t *self) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");

  axis_listnode_t *node = axis_list_pop_front(self);
  axis_listnode_destroy(node);
}

axis_listnode_t *axis_list_pop_back(axis_list_t *self) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");

  if (axis_list_is_empty(self)) {
    return NULL;
  }
  axis_listnode_t *node = self->back;
  if (axis_list_size(self) == 1) {
    self->front = self->back = NULL;
    node->prev = node->next = NULL;
  } else {
    self->back = self->back->prev;
    self->back->next = NULL;

    node->prev = NULL;
  }
  --self->size;

  return node;
}

bool axis_list_push_back_in_order(axis_list_t *self, axis_listnode_t *node,
                                 axis_list_node_compare_func_t cmp,
                                 bool skip_if_same) {
  axis_ASSERT(self && axis_list_check_integrity(self) && node && cmp,
             "Invalid argument.");

  if (axis_list_is_empty(self)) {
    axis_list_push_back(self, node);
    return true;
  }

  axis_list_foreach (self, iter) {
    int result = cmp(iter.node, node);
    if (result == 0 && skip_if_same) {
      return false;
    }

    if (result > 0) {
      continue;
    }

    if (iter.node == axis_list_front(self)) {
      axis_list_push_front(self, node);
      return true;
    }

    iter.prev->next = node;
    node->prev = iter.prev;
    node->next = iter.node;
    iter.node->prev = node;
    ++self->size;
    return true;
  }

  axis_list_push_back(self, node);
  return true;
}

axis_list_iterator_t axis_list_begin(axis_list_t *self) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");

  return (axis_list_iterator_t){
      NULL,
      axis_list_front(self),
      axis_list_front(self) ? axis_list_front(self)->next : NULL,
      0,
  };
}

axis_list_iterator_t axis_list_end(axis_list_t *self) {
  axis_ASSERT(self && axis_list_check_integrity(self), "Invalid argument.");

  if (axis_list_size(self)) {
    return (axis_list_iterator_t){
        axis_list_back(self) ? axis_list_back(self)->prev : NULL,
        axis_list_back(self),
        NULL,
        axis_list_size(self) - 1,
    };
  } else {
    return (axis_list_iterator_t){
        NULL,
        NULL,
        NULL,
        0,
    };
  }
}

axis_list_iterator_t axis_list_iterator_next(axis_list_iterator_t self) {
  return (axis_list_iterator_t){self.node, self.next,
                               self.next ? (self.next)->next : NULL,
                               self.index + 1};
}

axis_list_iterator_t axis_list_iterator_prev(axis_list_iterator_t self) {
  return (axis_list_iterator_t){self.prev ? (self.prev)->prev : NULL, self.prev,
                               self.node, self.index > 0 ? self.index - 1 : 0};
}

bool axis_list_iterator_is_end(axis_list_iterator_t self) {
  return self.node == NULL ? true : false;
}

axis_listnode_t *axis_list_iterator_to_listnode(axis_list_iterator_t self) {
  return self.node;
}

void axis_list_reverse_new(axis_list_t *src, axis_list_t *dest) {
  axis_ASSERT(src && axis_list_check_integrity(src), "Invalid argument.");
  axis_ASSERT(dest && axis_list_check_integrity(dest), "Invalid argument.");

  axis_list_foreach (src, iter) {
    axis_listnode_t *node = iter.node;
    axis_ASSERT(node, "Invalid argument.");

    axis_list_push_front(dest, node);
  }

  axis_list_init(src);
}

void axis_list_reverse(axis_list_t *src) {
  axis_ASSERT(src && axis_list_check_integrity(src), "Invalid argument.");

  axis_list_t dest = axis_LIST_INIT_VAL;
  axis_list_foreach (src, iter) {
    axis_listnode_t *node = iter.node;
    axis_ASSERT(node, "Invalid argument.");

    axis_list_push_front(&dest, node);
  }

  axis_list_init(src);

  axis_list_swap(&dest, src);
}
