//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
/* Modified from https://github.com/troydhanson/uthash. */
#pragma once

#include "axis_utils/axis_config.h"

#include <stdint.h>

typedef struct axis_hashtable_t axis_hashtable_t;
typedef struct axis_hashhandle_t axis_hashhandle_t;

struct axis_hashhandle_t {
  axis_hashtable_t *tbl;

  void *prev;  // previous hash handle in app-ordered list
  void *next;  // next hash handle in app-ordered list

  axis_hashhandle_t *hh_prev;  // previous item in bucket
  axis_hashhandle_t *hh_next;  // next item in bucket

  const void *key;   // ptr to key data
  uint32_t keylen;   // key len
  uint32_t hashval;  // result of hash function

  void (*destroy)(axis_hashhandle_t *);
};

axis_UTILS_API void axis_hashhandle_init(axis_hashhandle_t *self,
                                       axis_hashtable_t *table, const void *key,
                                       uint32_t keylen, void *destroy);

axis_UTILS_API void axis_hashhandle_del_from_app_list(axis_hashhandle_t *hh);
