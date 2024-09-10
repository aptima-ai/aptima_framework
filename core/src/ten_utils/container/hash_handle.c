//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/container/hash_handle.h"

#include <assert.h>

#include "ten_utils/container/hash_bucket.h"
#include "ten_utils/container/hash_table.h"

void ten_hashhandle_init(ten_hashhandle_t *self, ten_hashtable_t *table,
                         const void *key, uint32_t keylen, void *destroy) {
  assert(self && table && key);

  self->tbl = table;
  self->key = key;
  self->keylen = keylen;
  self->hashval = ten_hash_function(key, keylen);
  self->destroy = destroy;
}

void ten_hashhandle_del_from_app_list(ten_hashhandle_t *hh) {
  assert(hh);

  if (hh == hh->tbl->head) {
    if (hh->next) {
      hh->tbl->head = FIELD_OF_FROM_OFFSET(hh->next, hh->tbl->hh_offset);
    } else {
      hh->tbl->head = NULL;
    }
  }
  if (hh == hh->tbl->tail) {
    if (hh->prev) {
      hh->tbl->tail = FIELD_OF_FROM_OFFSET(hh->prev, hh->tbl->hh_offset);
    } else {
      hh->tbl->tail = NULL;
    }
  }
  if (hh->prev) {
    ((ten_hashhandle_t *)FIELD_OF_FROM_OFFSET(hh->prev, hh->tbl->hh_offset))
        ->next = hh->next;
  }
  if (hh->next) {
    ((ten_hashhandle_t *)FIELD_OF_FROM_OFFSET(hh->next, hh->tbl->hh_offset))
        ->prev = hh->prev;
  }
}
