//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/container/list_node.h"
#include "aptima_utils/lib/signature.h"

#define aptima_NORMAL_PTR_LISTNODE_SIGNATURE 0xEBB1285007CA4A12U

typedef void (*aptima_ptr_listnode_destroy_func_t)(void *ptr);

typedef struct aptima_ptr_listnode_t {
  aptima_listnode_t hdr;
  aptima_signature_t signature;
  void *ptr;
  aptima_ptr_listnode_destroy_func_t destroy;
} aptima_ptr_listnode_t;

aptima_UTILS_API aptima_listnode_t *aptima_ptr_listnode_create(
    void *ptr, aptima_ptr_listnode_destroy_func_t destroy);

aptima_UTILS_API aptima_ptr_listnode_t *aptima_listnode_to_ptr_listnode(
    aptima_listnode_t *self);

aptima_UTILS_API aptima_listnode_t *aptima_listnode_from_ptr_listnode(
    aptima_ptr_listnode_t *self);

aptima_UTILS_API void *aptima_ptr_listnode_get(aptima_listnode_t *self);

aptima_UTILS_API void aptima_ptr_listnode_replace(
    aptima_listnode_t *self, void *ptr, aptima_ptr_listnode_destroy_func_t destroy);
