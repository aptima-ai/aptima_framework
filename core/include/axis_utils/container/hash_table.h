//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
/* Modified from https://github.com/troydhanson/uthash. */
#pragma once

#include "axis_utils/axis_config.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "axis_utils/macro/field.h"

#define axis_hashtable_foreach(table, iter)                                     \
  for (axis_hashtable_iterator_t iter =                                         \
           {NULL, axis_hashtable_front(table),                                  \
            axis_hashtable_front(table)                                         \
                ? axis_hashtable_front(table)->next                             \
                      ? FIELD_OF_FROM_OFFSET(axis_hashtable_front(table)->next, \
                                             (table)->hh_offset)               \
                      : NULL                                                   \
                : NULL,                                                        \
            0};                                                                \
       (iter).node;                                                            \
       ++((iter).index), (iter).prev = (iter).node, (iter).node = (iter).next, \
                                (iter).next =                                  \
                                    (iter).node                                \
                                        ? ((iter).node)->next                  \
                                              ? FIELD_OF_FROM_OFFSET(          \
                                                    ((iter).node)->next,       \
                                                    (table)->hh_offset)        \
                                              : NULL                           \
                                        : NULL)

typedef struct axis_hashtable_t axis_hashtable_t;
typedef struct axis_hashbucket_t axis_hashbucket_t;
typedef struct axis_hashhandle_t axis_hashhandle_t;

struct axis_hashtable_t {
  axis_hashbucket_t *bkts;

  axis_hashhandle_t *head;  // App-ordered list head.
  axis_hashhandle_t *tail;  // App-ordered list tail.
  ptrdiff_t hh_offset;

  uint32_t bkts_cnt;
  uint32_t bkts_cnt_in_log2;
  uint32_t items_cnt;

  // In a ideal situation (all buckets used equally), no bucket would have
  // more than ceil (#items/#buckets) items. that's the ideal chain length.
  uint32_t ideal_chain_maxlen;

  // non_ideal_items_cnt is the number of items in the hash whose chain position
  // exceeds the ideal chain maxlen. These items pay the penalty for a uneven
  // hash distribution; reaching them in a chain traversal takes >ideal steps.
  uint32_t non_ideal_items_cnt;

  // ineffective expands occur when a bucket doubling was performed, but
  // afterward, more than half the items in the hash had non_ideal chain
  // positions. If this happens on two consecutive expansions we inhibit any
  // further expansion, as it's not helping; this happens when the hash
  // function isn't a good fit for the key domain. When expansion is inhibited
  // the hash will still work, albeit no longer in constant time. */
  uint32_t ineff_expands_times;
  bool noexpand;
};

typedef struct axis_hashtable_iterator_t {
  axis_hashhandle_t *prev;
  axis_hashhandle_t *node;
  axis_hashhandle_t *next;
  size_t index;
} axis_hashtable_iterator_t;

axis_UTILS_API uint32_t axis_hash_function(const void *key, uint32_t keylen);

/**
 * @brief Create a hash table.
 */
axis_UTILS_API axis_hashtable_t *axis_hashtable_create(ptrdiff_t hh_offset);

/**
 * @brief Destroy a hash table.
 */
axis_UTILS_API void axis_hashtable_destroy(axis_hashtable_t *self);

axis_UTILS_API uint32_t axis_hashtable_items_cnt(axis_hashtable_t *self);

axis_UTILS_API void axis_hashtable_init(axis_hashtable_t *self,
                                      ptrdiff_t hh_offset);

axis_UTILS_API void axis_hashtable_deinit(axis_hashtable_t *self);

axis_UTILS_API void axis_hashtable_clear(axis_hashtable_t *self);

/**
 * @brief Concatenate the second hash table into the first one.
 * @param self The hash table to be concatenated.
 * @param target The hash table to be concatenating to the first one.
 */
axis_UTILS_API void axis_hashtable_concat(axis_hashtable_t *self,
                                        axis_hashtable_t *target);

axis_UTILS_API void axis_hashtable_expand_bkts(axis_hashtable_t *self);

axis_UTILS_API void axis_hashtable_add_by_key(axis_hashtable_t *self,
                                            axis_hashhandle_t *hh,
                                            const void *key, uint32_t keylen,
                                            void *destroy);

axis_UTILS_API void axis_hashtable_replace_by_key(axis_hashtable_t *self,
                                                axis_hashhandle_t *hh,
                                                void *keyptr,
                                                uint32_t keylen_in,
                                                void *destroy);

axis_UTILS_API void axis_hashtable_del(axis_hashtable_t *self,
                                     axis_hashhandle_t *hh);

axis_UTILS_API axis_hashhandle_t *axis_hashtable_front(axis_hashtable_t *self);

axis_UTILS_API axis_hashhandle_t *axis_hashtable_back(axis_hashtable_t *self);

axis_UTILS_API axis_hashhandle_t *axis_hashtable_find_by_key(axis_hashtable_t *self,
                                                          const void *key,
                                                          uint32_t keylen);

axis_UTILS_API axis_hashhandle_t *axis_hashtable_find(axis_hashtable_t *self,
                                                   uint32_t hashval,
                                                   const void *key,
                                                   uint32_t keylen);

/**
 * @brief Add an item to the hash table with string-type key.
 */
static inline void axis_hashtable_add_string(axis_hashtable_t *self,
                                            axis_hashhandle_t *hh,
                                            const char *str, void *destroy) {
  assert(self && hh && str);
  axis_hashtable_add_by_key(self, hh, str, strlen(str), destroy);
}

static inline void axis_hashtable_replace_string(axis_hashtable_t *self,
                                                axis_hashhandle_t *hh, char *str,
                                                void *destroy) {
  assert(self);
  axis_hashtable_replace_by_key(self, hh, str, strlen(str), destroy);
}

static inline axis_hashhandle_t *axis_hashtable_find_string(axis_hashtable_t *self,
                                                          const char *str) {
  assert(self && str);
  return axis_hashtable_find_by_key(self, str, strlen(str));
}

/**
 * @brief Add an item to the hash table with integer-type key.
 */
static inline void axis_hashtable_add_int(axis_hashtable_t *self,
                                         axis_hashhandle_t *hh, int32_t *value,
                                         void *destroy) {
  assert(self && hh);
  axis_hashtable_add_by_key(self, hh, value, sizeof(int32_t), destroy);
}

static inline void axis_hashtable_replace_int(axis_hashtable_t *self,
                                             axis_hashhandle_t *hh,
                                             int32_t *value, void *destroy) {
  assert(self && hh);
  axis_hashtable_replace_by_key(self, hh, value, sizeof(int32_t), destroy);
}

static inline axis_hashhandle_t *axis_hashtable_find_int(axis_hashtable_t *self,
                                                       int32_t *value) {
  assert(self);
  return axis_hashtable_find_by_key(self, value, sizeof(int32_t));
}

/**
 * @brief Add an item to the hash table with pointer-type key.
 */
static inline void axis_hashtable_add_ptr(axis_hashtable_t *self,
                                         axis_hashhandle_t *hh, void *ptr,
                                         void *destroy) {
  assert(self && hh);
  axis_hashtable_add_by_key(self, hh, &ptr, sizeof(void *), destroy);
}

static inline void axis_hashtable_replace_ptr(axis_hashtable_t *self,
                                             axis_hashhandle_t *hh, void *ptr,
                                             void *destroy) {
  assert(self && hh);
  axis_hashtable_replace_by_key(self, hh, &ptr, sizeof(void *), destroy);
}

static inline axis_hashhandle_t *axis_hashtable_find_ptr(axis_hashtable_t *self,
                                                       void *ptr) {
  assert(self);
  return axis_hashtable_find_by_key(self, &ptr, sizeof(void *));
}
