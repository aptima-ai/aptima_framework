//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg/locked_res.h"

#include "include_internal/axis_runtime/msg/msg.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_ptr.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"

static bool axis_msg_locked_res_check_integrity(axis_msg_locked_res_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_MSG_LOCKED_RES_SIGNATURE) {
    return false;
  }
  return true;
}

static void axis_msg_locked_res_init(axis_msg_locked_res_t *self,
                                    axis_MSG_LOCKED_RES_TYPE type) {
  axis_ASSERT(self, "Should not happen.");

  axis_signature_set(&self->signature, axis_MSG_LOCKED_RES_SIGNATURE);
  self->type = type;
}

static axis_msg_locked_res_buf_t *axis_msg_locked_res_buf_create(
    const uint8_t *data) {
  axis_msg_locked_res_buf_t *res =
      (axis_msg_locked_res_buf_t *)axis_MALLOC(sizeof(axis_msg_locked_res_buf_t));
  axis_ASSERT(res, "Failed to allocate axis_msg_locked_res_buf_t.");

  axis_msg_locked_res_init((axis_msg_locked_res_t *)res,
                          axis_MSG_LOCKED_RES_TYPE_BUF);

  res->data = data;

  return res;
}

static void axis_msg_locked_res_buf_destroy(axis_msg_locked_res_buf_t *info) {
  axis_ASSERT(info && axis_msg_locked_res_check_integrity(&info->base),
             "Invalid argument.");

  info->data = NULL;

  axis_FREE(info);
}

static void axis_raw_msg_add_locked_res_buf(axis_msg_t *self,
                                           const uint8_t *data) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Invalid argument.");

  axis_msg_locked_res_buf_t *res = axis_msg_locked_res_buf_create(data);
  axis_ASSERT(res && axis_msg_locked_res_check_integrity(&res->base),
             "Should not happen.");

  axis_list_push_ptr_back(
      &self->locked_res, res,
      (axis_ptr_listnode_destroy_func_t)axis_msg_locked_res_buf_destroy);
}

bool axis_raw_msg_remove_locked_res_buf(axis_msg_t *self, const uint8_t *data) {
  axis_ASSERT(self && axis_raw_msg_check_integrity(self), "Invalid argument.");

  if (!data) {
    return false;
  }

  axis_list_foreach (&self->locked_res, iter) {
    axis_msg_locked_res_buf_t *res = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(res && axis_msg_locked_res_check_integrity(&res->base),
               "Should not happen.");

    if (res->data == data) {
      axis_list_remove_node(&self->locked_res, iter.node);
      return true;
    }
  }

  return false;
}

bool axis_msg_add_locked_res_buf(axis_shared_ptr_t *self, const uint8_t *data,
                                axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Invalid argument.");

  if (!data) {
    axis_LOGE("Failed to lock res, the data is null.");
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT, "Try to lock a NULL buf.");
    }
    return false;
  }

  axis_raw_msg_add_locked_res_buf(axis_msg_get_raw_msg(self), data);

  return true;
}

bool axis_msg_remove_locked_res_buf(axis_shared_ptr_t *self, const uint8_t *data,
                                   axis_error_t *err) {
  axis_ASSERT(self && axis_msg_check_integrity(self), "Invalid argument.");

  bool result =
      axis_raw_msg_remove_locked_res_buf(axis_msg_get_raw_msg(self), data);

  if (!result) {
    axis_LOGE("Fatal, the locked res %p is not found.", data);
    if (err) {
      axis_error_set(err, axis_ERRNO_INVALID_ARGUMENT,
                    "Failed to remove locked res.");
    }
  }

  return result;
}
