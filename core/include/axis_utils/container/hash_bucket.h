//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
/* Modified from https://github.com/troydhanson/uthash. */
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HASH_BKT_CAPACITY_THRESH 10U  // Expand when bucket items reaches

typedef struct axis_hashbucket_t axis_hashbucket_t;
typedef struct axis_hashhandle_t axis_hashhandle_t;

struct axis_hashbucket_t {
  axis_hashhandle_t *head;
  uint32_t items_cnt;

  // expand_mult is normally set to 0. In this situation, the max chain length
  // threshold is enforced at its default value, HASH_BKT_CAPACITY_THRESH. (If
  // the bucket's chain exceeds this length, bucket expansion is triggered).
  // However, setting expand_mult to a non-zero value delays bucket expansion
  // (that would be triggered by additions to this particular bucket)
  // until its chain length reaches a *multiple* of HASH_BKT_CAPACITY_THRESH.
  // (The multiplier is simply expand_mult+1). The whole idea of this
  // multiplier is to reduce bucket expansions, since they are expensive, in
  // situations where we know that a particular bucket tends to be overused.
  // It is better to let its chain length grow to a longer yet-still-bounded
  // value, than to do a O(n) bucket expansion too often.
  uint32_t expand_mult;
};

axis_UTILS_API void axis_hashbucket_add(axis_hashbucket_t *self,
                                      axis_hashhandle_t *hh);

axis_UTILS_API void axis_hashbucket_del(axis_hashbucket_t *self,
                                      axis_hashhandle_t *hh);

axis_UTILS_API axis_hashhandle_t *axis_hashbucket_find(axis_hashbucket_t *self,
                                                    uint32_t hashval,
                                                    const void *key,
                                                    size_t keylen);
