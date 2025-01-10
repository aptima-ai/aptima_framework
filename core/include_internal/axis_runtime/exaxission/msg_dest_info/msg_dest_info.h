//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/path/result_return_policy.h"
#include "axis_utils/container/list.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"

#define axis_MSG_DEST_STATIC_INFO_SIGNATURE 0x43B5CAAF1BB9BC41U
#define axis_MSG_DEST_RUNTIME_INFO_SIGNATURE 0x834F4E128C7658EEU

typedef struct axis_msg_dest_info_t {
  axis_signature_t signature;
  axis_string_t name;  // The name of a message or an interface.
  axis_RESULT_RETURN_POLICY policy;
  axis_list_t dest;  // axis_weak_ptr_t of axis_extension_info_t
} axis_msg_dest_info_t;

axis_RUNTIME_PRIVATE_API bool axis_msg_dest_info_check_integrity(
    axis_msg_dest_info_t *self);

axis_RUNTIME_PRIVATE_API axis_msg_dest_info_t *axis_msg_dest_info_create(
    const char *msg_name);

axis_RUNTIME_PRIVATE_API axis_shared_ptr_t *axis_msg_dest_info_clone(
    axis_shared_ptr_t *self, axis_list_t *extensions_info, axis_error_t *err);

axis_RUNTIME_PRIVATE_API void axis_msg_dest_info_destroy(
    axis_msg_dest_info_t *self);

axis_RUNTIME_PRIVATE_API void axis_msg_dest_info_translate_localhost_to_app_uri(
    axis_msg_dest_info_t *self, const char *uri);

axis_RUNTIME_PRIVATE_API bool axis_msg_dest_info_qualified(
    axis_msg_dest_info_t *self, const char *msg_name);
