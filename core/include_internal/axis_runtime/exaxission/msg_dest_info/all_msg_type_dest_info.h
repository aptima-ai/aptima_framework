//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/container/list.h"

typedef struct axis_extension_t axis_extension_t;

typedef struct axis_all_msg_type_dest_info_t {
  axis_list_t cmd;          // axis_shared_ptr_t of axis_msg_dest_info_t
  axis_list_t data;         // axis_shared_ptr_t of axis_msg_dest_info_t
  axis_list_t video_frame;  // axis_shared_ptr_t of axis_msg_dest_info_t
  axis_list_t audio_frame;  // axis_shared_ptr_t of axis_msg_dest_info_t
  axis_list_t interface;    // axis_shared_ptr_t of axis_msg_dest_info_t
} axis_all_msg_type_dest_info_t;

axis_RUNTIME_PRIVATE_API void axis_all_msg_type_dest_info_init(
    axis_all_msg_type_dest_info_t *self);

axis_RUNTIME_PRIVATE_API void axis_all_msg_type_dest_info_deinit(
    axis_all_msg_type_dest_info_t *self);

axis_RUNTIME_PRIVATE_API void
axis_all_msg_type_dest_info_translate_localhost_to_app_uri(
    axis_all_msg_type_dest_info_t *self, const char *uri);
