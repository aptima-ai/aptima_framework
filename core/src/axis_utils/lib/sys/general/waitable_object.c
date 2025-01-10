//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/waitable_object.h"

#include <memory.h>
#include <stdint.h>
#include <stdlib.h>

#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/cond.h"
#include "axis_utils/lib/mutex.h"
#include "axis_utils/lib/waitable_number.h"

typedef struct axis_waitable_object_t {
  union {
    void *obj;
    volatile int64_t num;
  } v, p;
  int (*compare)(const void *l, const void *r);
  axis_cond_t *cond;
  axis_mutex_t *lock;
} axis_waitable_number_t, axis_waitable_object_t;

static void axis_waitable_object_free(axis_waitable_object_t *obj) {
  if (obj) {
    if (obj->lock) {
      axis_mutex_destroy(obj->lock);
      obj->lock = NULL;
    }

    if (obj->cond) {
      axis_cond_destroy(obj->cond);
      obj->cond = NULL;
    }

    axis_free(obj);
  }
}

static axis_waitable_number_t *axis_waitable_object_alloc() {
  axis_waitable_object_t *obj = NULL;

  obj = (axis_waitable_object_t *)axis_malloc(sizeof(*obj));
  if (!obj) {
    goto error;
  }

  memset(obj, 0, sizeof(*obj));

  obj->cond = axis_cond_create();
  if (!obj->cond) {
    goto error;
  }

  obj->lock = axis_mutex_create();
  if (!obj->lock) {
    goto error;
  }

  return obj;

error:
  axis_waitable_object_free(obj);
  return NULL;
}

static int axis_waitable_object_valid(axis_waitable_object_t *obj) {
  return (obj && obj->cond && obj->lock) ? 1 : 0;
}

static int axis_number_is_equal(void *arg) {
  axis_waitable_object_t *obj = (axis_waitable_object_t *)arg;
  return obj->v.num == obj->p.num ? 1 : 0;
}

static int axis_number_is_not_equal(void *arg) {
  axis_waitable_object_t *obj = (axis_waitable_object_t *)arg;
  return obj->v.num == obj->p.num ? 0 : 1;
}

static int axis_obj_is_equal(void *arg) {
  axis_waitable_object_t *obj = (axis_waitable_object_t *)arg;
  return obj->compare(obj->v.obj, obj->p.obj);
}

static int axis_obj_is_not_equal(void *arg) {
  axis_waitable_object_t *obj = (axis_waitable_object_t *)arg;
  return !obj->compare(obj->v.obj, obj->p.obj);
}

axis_waitable_number_t *axis_waitable_number_create(int64_t init_value) {
  axis_waitable_object_t *obj = axis_waitable_object_alloc();
  if (!obj) {
    return NULL;
  }

  obj->v.num = init_value;
  return obj;
}

void axis_waitable_number_destroy(axis_waitable_number_t *number) {
  axis_waitable_object_free(number);
}

void axis_waitable_number_increase(axis_waitable_number_t *number,
                                  int64_t value) {
  if (!axis_waitable_object_valid(number)) {
    return;
  }

  if (value == 0) {
    return;
  }

  axis_mutex_lock(number->lock);
  number->v.num += value;
  axis_cond_broadcast(number->cond);
  axis_mutex_unlock(number->lock);
}

void axis_waitable_number_decrease(axis_waitable_number_t *number,
                                  int64_t value) {
  if (!axis_waitable_object_valid(number)) {
    return;
  }

  if (value == 0) {
    return;
  }

  axis_mutex_lock(number->lock);
  number->v.num -= value;
  axis_cond_broadcast(number->cond);
  axis_mutex_unlock(number->lock);
}

void axis_waitable_number_multiply(axis_waitable_number_t *number,
                                  int64_t value) {
  if (!axis_waitable_object_valid(number)) {
    return;
  }

  if (value == 1) {
    return;
  }

  axis_mutex_lock(number->lock);
  number->v.num *= value;
  axis_cond_broadcast(number->cond);
  axis_mutex_unlock(number->lock);
}

void axis_waitable_number_divide(axis_waitable_number_t *number, int64_t value) {
  if (!axis_waitable_object_valid(number)) {
    return;
  }

  if (value == 1) {
    return;
  }

  axis_mutex_lock(number->lock);
  number->v.num /= value;
  axis_cond_broadcast(number->cond);
  axis_mutex_unlock(number->lock);
}

void axis_waitable_number_set(axis_waitable_number_t *number, int64_t value) {
  if (!axis_waitable_object_valid(number)) {
    return;
  }

  axis_mutex_lock(number->lock);
  if (number->v.num != value) {
    number->v.num = value;
    axis_cond_broadcast(number->cond);
  }
  axis_mutex_unlock(number->lock);
}

int64_t axis_waitable_number_get(axis_waitable_number_t *number) {
  int64_t ret = 0;

  if (!axis_waitable_object_valid(number)) {
    return 0;
  }

  axis_mutex_lock(number->lock);
  ret = number->v.num;
  axis_mutex_unlock(number->lock);

  return ret;
}

int axis_waitable_number_wait_until(axis_waitable_number_t *number, int64_t value,
                                   int timeout) {
  int ret = -1;

  if (!axis_waitable_object_valid(number)) {
    return -1;
  }

  axis_mutex_lock(number->lock);
  number->p.num = value;
  ret = axis_cond_wait_while(number->cond, number->lock, axis_number_is_not_equal,
                            number, timeout);
  number->p.num = 0;
  axis_mutex_unlock(number->lock);
  return ret;
}

int axis_waitable_number_wait_while(axis_waitable_number_t *number, int64_t value,
                                   int timeout) {
  int ret = -1;

  if (!axis_waitable_object_valid(number)) {
    return -1;
  }

  axis_mutex_lock(number->lock);
  number->p.num = value;
  ret = axis_cond_wait_while(number->cond, number->lock, axis_number_is_equal,
                            number, timeout);
  number->p.num = 0;
  axis_mutex_unlock(number->lock);
  return ret;
}

axis_waitable_object_t *axis_waitable_object_create(void *init_value) {
  axis_waitable_object_t *obj = axis_waitable_object_alloc();
  if (!obj) {
    return NULL;
  }

  obj->v.obj = init_value;
  return obj;
}

void axis_waitable_object_destroy(axis_waitable_object_t *obj) {
  axis_waitable_object_free(obj);
}

void axis_waitable_object_set(axis_waitable_object_t *obj, void *value) {
  if (!axis_waitable_object_valid(obj)) {
    return;
  }

  axis_mutex_lock(obj->lock);
  obj->v.obj = value;
  axis_cond_broadcast(obj->cond);
  axis_mutex_unlock(obj->lock);
}

void axis_waitable_object_update(axis_waitable_object_t *obj) {
  if (!axis_waitable_object_valid(obj)) {
    return;
  }

  axis_mutex_lock(obj->lock);
  axis_cond_broadcast(obj->cond);
  axis_mutex_unlock(obj->lock);
}

void *axis_waitable_object_get(axis_waitable_object_t *obj) {
  void *ret = 0;

  if (!axis_waitable_object_valid(obj)) {
    return NULL;
  }

  axis_mutex_lock(obj->lock);
  ret = obj->v.obj;
  axis_mutex_unlock(obj->lock);

  return ret;
}

int axis_waitable_object_wait_until(axis_waitable_object_t *obj,
                                   int (*compare)(const void *l, const void *r),
                                   int timeout) {
  int ret = -1;

  if (!axis_waitable_object_valid(obj)) {
    return -1;
  }

  axis_mutex_lock(obj->lock);
  obj->compare = compare;
  ret = axis_cond_wait_while(obj->cond, obj->lock, axis_obj_is_not_equal, obj,
                            timeout);
  obj->compare = NULL;
  axis_mutex_unlock(obj->lock);
  return ret;
}

int axis_waitable_object_wait_while(axis_waitable_object_t *obj,
                                   int (*compare)(const void *l, const void *r),
                                   int timeout) {
  int ret = -1;

  if (!axis_waitable_object_valid(obj)) {
    return -1;
  }

  axis_mutex_lock(obj->lock);
  obj->compare = compare;
  ret =
      axis_cond_wait_while(obj->cond, obj->lock, axis_obj_is_equal, obj, timeout);
  obj->compare = NULL;
  axis_mutex_unlock(obj->lock);
  return ret;
}