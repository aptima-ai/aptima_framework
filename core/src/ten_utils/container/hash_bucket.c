//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/container/hash_bucket.h"

#include <assert.h>

#include "ten_utils/container/hash_handle.h"
#include "ten_utils/container/hash_table.h"
#include "ten_utils/macro/macros.h"

void ten_hashbucket_add(ten_hashbucket_t *self, ten_hashhandle_t *hh) {
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
    ten_hashtable_expand_bkts(hh->tbl);
  }
}

// Remove a item from a given bucket.
void ten_hashbucket_del(ten_hashbucket_t *self, ten_hashhandle_t *hh) {
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

ten_hashhandle_t *ten_hashbucket_find(ten_hashbucket_t *self, uint32_t hashval,
                                      const void *key, size_t keylen) {
  assert(self && key);

  ten_hashhandle_t *out = NULL;
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
