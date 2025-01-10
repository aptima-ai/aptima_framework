//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/container/hash_bucket.h"

#include <assert.h>

#include "axis_utils/container/hash_handle.h"
#include "axis_utils/container/hash_table.h"

void axis_hashbucket_add(axis_hashbucket_t *self, axis_hashhandle_t *hh) {
  assert(self && hh);

  self->items_cnt++;

  // Insert to the front of the bucket.
  hh->hh_next = self->head;
  hh->hh_prev = NULL;
  if (self->head != NULL) {
    self->head->hh_prev = hh;
  }
  self->head = hh;

  if ((self->items_cnt >=
       ((self->expand_mult + 1U) * HASH_BKT_CAPACITY_THRESH)) &&
      !hh->tbl->noexpand) {
    axis_hashtable_expand_bkts(hh->tbl);
  }
}

// Remove a item from a given bucket.
void axis_hashbucket_del(axis_hashbucket_t *self, axis_hashhandle_t *hh) {
  assert(self && hh);

  self->items_cnt--;

  // Remove from the bucket.
  if (self->head == hh) {
    self->head = hh->hh_next;
  }
  if (hh->hh_prev) {
    hh->hh_prev->hh_next = hh->hh_next;
  }
  if (hh->hh_next) {
    hh->hh_next->hh_prev = hh->hh_prev;
  }
}

axis_hashhandle_t *axis_hashbucket_find(axis_hashbucket_t *self, uint32_t hashval,
                                      const void *key, size_t keylen) {
  assert(self && key);

  axis_hashhandle_t *out = NULL;
  if (self->head != NULL) {
    out = self->head;
  }
  while (out != NULL) {
    if (out->hashval == hashval && out->keylen == keylen) {
      if (memcmp(out->key, key, keylen) == 0) {
        break;
      }
    }
    if (out->hh_next != NULL) {
      out = out->hh_next;
    } else {
      out = NULL;
    }
  }
  return out;
}
