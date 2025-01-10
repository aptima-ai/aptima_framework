//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/extension/msg_dest_info/all_msg_type_dest_info.h"

#include "include_internal/axis_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/macro/check.h"

void axis_all_msg_type_dest_info_init(axis_all_msg_type_dest_info_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_list_init(&self->cmd);
  axis_list_init(&self->video_frame);
  axis_list_init(&self->audio_frame);
  axis_list_init(&self->data);
  axis_list_init(&self->interface);
}

void axis_all_msg_type_dest_info_deinit(axis_all_msg_type_dest_info_t *self) {
  axis_ASSERT(self, "Should not happen.");

  axis_list_clear(&self->cmd);
  axis_list_clear(&self->video_frame);
  axis_list_clear(&self->audio_frame);
  axis_list_clear(&self->data);
  axis_list_clear(&self->interface);
}

static void translate_localhost_to_app_uri_for_msg_dest(axis_shared_ptr_t *dest,
                                                        const char *uri) {
  axis_msg_dest_info_t *raw_dest = axis_shared_ptr_get_data(dest);
  axis_msg_dest_info_translate_localhost_to_app_uri(raw_dest, uri);
}

static void translate_localhost_to_app_uri_for_dest(
    axis_listnode_t *node, const char *uri,
    void (*translate_func)(axis_shared_ptr_t *dest, const char *uri)) {
  axis_ASSERT(node, "Invalid argument.");
  axis_ASSERT(uri, "Invalid argument.");
  axis_ASSERT(translate_func, "Invalid argument.");

  axis_shared_ptr_t *shared_dest = axis_smart_ptr_listnode_get(node);
  axis_ASSERT(shared_dest, "Should not happen.");

  translate_func(shared_dest, uri);
}

void axis_all_msg_type_dest_info_translate_localhost_to_app_uri(
    axis_all_msg_type_dest_info_t *self, const char *uri) {
  axis_ASSERT(self && uri, "Invalid argument.");

  axis_list_foreach (&self->cmd, iter) {
    translate_localhost_to_app_uri_for_dest(
        iter.node, uri, translate_localhost_to_app_uri_for_msg_dest);
  }

  axis_list_foreach (&self->data, iter) {
    translate_localhost_to_app_uri_for_dest(
        iter.node, uri, translate_localhost_to_app_uri_for_msg_dest);
  }

  axis_list_foreach (&self->video_frame, iter) {
    translate_localhost_to_app_uri_for_dest(
        iter.node, uri, translate_localhost_to_app_uri_for_msg_dest);
  }

  axis_list_foreach (&self->audio_frame, iter) {
    translate_localhost_to_app_uri_for_dest(
        iter.node, uri, translate_localhost_to_app_uri_for_msg_dest);
  }
}
