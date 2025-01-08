//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/signature.h"

#define axis_NORMAL_PTR_LISTNODE_SIGNATURE 0xEBB1285007CA4A12U

typedef void (*axis_ptr_listnode_destroy_func_t)(void *ptr);

typedef struct axis_ptr_listnode_t {
  axis_listnode_t hdr;
  axis_signature_t signature;
  void *ptr;
  axis_ptr_listnode_destroy_func_t destroy;
} axis_ptr_listnode_t;

axis_UTILS_API axis_listnode_t *axis_ptr_listnode_create(
    void *ptr, axis_ptr_listnode_destroy_func_t destroy);

axis_UTILS_API axis_ptr_listnode_t *axis_listnode_to_ptr_listnode(
    axis_listnode_t *self);

axis_UTILS_API axis_listnode_t *axis_listnode_from_ptr_listnode(
    axis_ptr_listnode_t *self);

axis_UTILS_API void *axis_ptr_listnode_get(axis_listnode_t *self);

axis_UTILS_API void axis_ptr_listnode_replace(
    axis_listnode_t *self, void *ptr, axis_ptr_listnode_destroy_func_t destroy);
