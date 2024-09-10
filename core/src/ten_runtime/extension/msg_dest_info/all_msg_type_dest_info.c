//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/extension/msg_dest_info/all_msg_type_dest_info.h"

#include "include_internal/ten_runtime/extension/extension_hdr.h"
#include "include_internal/ten_runtime/extension/msg_dest_info/msg_dest_info.h"
#include "ten_runtime/extension/extension.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/container/list_smart_ptr.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/macro/check.h"

void ten_all_msg_type_dest_static_info_init(
    ten_all_msg_type_dest_static_info_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_list_init(&self->cmd);
  ten_list_init(&self->video_frame);
  ten_list_init(&self->audio_frame);
  ten_list_init(&self->data);
  ten_list_init(&self->interface);
}

void ten_all_msg_type_dest_static_info_deinit(
    ten_all_msg_type_dest_static_info_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_list_clear(&self->cmd);
  ten_list_clear(&self->video_frame);
  ten_list_clear(&self->audio_frame);
  ten_list_clear(&self->data);
  ten_list_clear(&self->interface);
}

static void translate_localhost_to_app_uri_for_msg_dest(ten_shared_ptr_t *dest,
                                                        const char *uri) {
  ten_msg_dest_static_info_t *raw_dest = ten_shared_ptr_get_data(dest);
  ten_msg_dest_static_info_translate_localhost_to_app_uri(raw_dest, uri);
}

static void translate_localhost_to_app_uri_for_dest(
    ten_listnode_t *node, const char *uri,
    void (*translate_func)(ten_shared_ptr_t *dest, const char *uri)) {
  TEN_ASSERT(node, "Invalid argument.");
  TEN_ASSERT(uri, "Invalid argument.");
  TEN_ASSERT(translate_func, "Invalid argument.");

  ten_shared_ptr_t *shared_dest = ten_smart_ptr_listnode_get(node);
  TEN_ASSERT(shared_dest, "Should not happen.");

  translate_func(shared_dest, uri);
}

void ten_all_msg_type_dest_static_info_translate_localhost_to_app_uri(
    ten_all_msg_type_dest_static_info_t *self, const char *uri) {
  TEN_ASSERT(self && uri, "Invalid argument.");

  ten_list_foreach (&self->cmd, iter) {
    translate_localhost_to_app_uri_for_dest(
        iter.node, uri, translate_localhost_to_app_uri_for_msg_dest);
  }

  ten_list_foreach (&self->data, iter) {
    translate_localhost_to_app_uri_for_dest(
        iter.node, uri, translate_localhost_to_app_uri_for_msg_dest);
  }

  ten_list_foreach (&self->video_frame, iter) {
    translate_localhost_to_app_uri_for_dest(
        iter.node, uri, translate_localhost_to_app_uri_for_msg_dest);
  }

  ten_list_foreach (&self->audio_frame, iter) {
    translate_localhost_to_app_uri_for_dest(
        iter.node, uri, translate_localhost_to_app_uri_for_msg_dest);
  }
}

void ten_all_msg_type_dest_runtime_info_init(
    ten_all_msg_type_dest_runtime_info_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_list_init(&self->cmd);
  ten_list_init(&self->video_frame);
  ten_list_init(&self->audio_frame);
  ten_list_init(&self->data);
  ten_list_init(&self->interface);
}

void ten_all_msg_type_dest_runtime_info_deinit(
    ten_all_msg_type_dest_runtime_info_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_list_clear(&self->cmd);
  ten_list_clear(&self->video_frame);
  ten_list_clear(&self->audio_frame);
  ten_list_clear(&self->data);
  ten_list_clear(&self->interface);
}

static void ten_direct_one_msg_type_to_extension(ten_list_t *msg_type_list,
                                                 ten_extension_t *extension) {
  TEN_ASSERT(msg_type_list, "Should not happen.");
  TEN_ASSERT(extension, "Should not happen.");

  ten_msg_dest_runtime_info_t *self_cmd = ten_msg_dest_runtime_info_create("*");
  ten_extensionhdr_t *self_extension_hdr =
      ten_extensionhdr_create_for_extension(extension);
  ten_list_push_ptr_back(
      &self_cmd->dest, self_extension_hdr,
      (ten_ptr_listnode_destroy_func_t)ten_extensionhdr_destroy);

  ten_shared_ptr_t *shared_self_cmd =
      ten_shared_ptr_create(self_cmd, ten_msg_dest_runtime_info_destroy);
  ten_list_push_smart_ptr_back(msg_type_list, shared_self_cmd);
  ten_shared_ptr_destroy(shared_self_cmd);
}

void ten_all_msg_type_dest_runtime_info_direct_all_msg_to_extension(
    ten_all_msg_type_dest_runtime_info_t *self, ten_extension_t *extension) {
  TEN_ASSERT(self, "Should not happen.");
  TEN_ASSERT(extension, "Should not happen.");

  ten_direct_one_msg_type_to_extension(&self->cmd, extension);
  ten_direct_one_msg_type_to_extension(&self->data, extension);
  ten_direct_one_msg_type_to_extension(&self->audio_frame, extension);
  ten_direct_one_msg_type_to_extension(&self->video_frame, extension);
}
