//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/binding/common.h"
#include "include_internal/axis_runtime/extension/msg_dest_info/all_msg_type_dest_info.h"
#include "include_internal/axis_runtime/extension/path_timer.h"
#include "include_internal/axis_runtime/metadata/metadata_info.h"
#include "include_internal/axis_runtime/schema_store/store.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/extension/extension.h"
#include "axis_utils/container/hash_handle.h"
#include "axis_utils/container/list.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/value/value.h"

#define axis_EXTENSION_SIGNATURE 0xE1627776E09A723CU

// In most modern operating systems, `-1` is not a valid user-space address.
// Therefore, we use this tricky approach to represent the value of a pointer to
// an extension that was not successfully created.
#define axis_EXTENSION_UNSUCCESSFULLY_CREATED ((axis_extension_t *)-1)

typedef struct axis_env_t axis_env_t;
typedef struct axis_extension_t axis_extension_t;
typedef struct axis_audio_frame_t axis_audio_frame_t;
typedef struct axis_video_frame_t axis_video_frame_t;
typedef struct axis_extension_info_t axis_extension_info_t;
typedef struct axis_extension_context_t axis_extension_context_t;
typedef struct axis_extension_group_t axis_extension_group_t;
typedef struct axis_extension_thread_t axis_extension_thread_t;
typedef struct axis_addon_host_t axis_addon_host_t;
typedef struct axis_path_table_t axis_path_table_t;
typedef struct axis_path_in_t axis_path_in_t;
typedef struct axis_timer_t axis_timer_t;

// The relationship between several lifecycle stages and their connection to
// sending messages:
//
// - on_configure ~ on_configure_done + on_init ~ on_init_done: Handles its own
//   initialization; cannot send or receive messages. The reason for this is
//   that, before `on_init_done`, the extension may not be ready to handle
//   external requests, so the received messages need to be temporarily stored.
//
// - ~ on_start: The messages received before on_start() will be temporarily
//   stored, and only after on_start() is called will they be sent to the
//   extension. The reason for this is developers generally expect `on_start` to
//   occur before any `on_cmd` events.
//
// - on_start ~ on_stop_done: Normal sending and receiving of all
//   messages and results.
//
// - on_deinit ~ on_deinit_done: Handles its own de-initialization; cannot
//   receive messages.
typedef enum axis_EXTENSION_STATE {
  axis_EXTENSION_STATE_INIT,

  // on_configure_done() is completed.
  axis_EXTENSION_STATE_ON_CONFIGURE_DONE,

  // on_init_done() is completed.
  axis_EXTENSION_STATE_ON_INIT_DONE,

  // on_start() is called.
  axis_EXTENSION_STATE_ON_START,

  // on_start_done() is completed.
  axis_EXTENSION_STATE_ON_START_DONE,

  // on_stop_done() is completed.
  axis_EXTENSION_STATE_ON_STOP_DONE,

  // on_deinit() is called.
  axis_EXTENSION_STATE_ON_DEINIT,

  // on_deinit_done() is called.
  axis_EXTENSION_STATE_ON_DEINIT_DONE,
} axis_EXTENSION_STATE;

struct axis_extension_t {
  axis_binding_handle_t binding_handle;

  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_EXTENSION_STATE state;

  // @{
  // Public interface.
  //
  // These public APIs are all async behaviors, that is to say, addon needs to
  // actively call on_xxx_done function to notify the APTIMA runtime that it is
  // done. But in some language bindings (such as JavaScript), because of
  // language-level asynchronous support (such as async/await), APTIMA runtime can
  // only provide async syntax API to addons (such as `async function onCmd`).
  // Addons can write async codes or sync codes in such async syntax API; but in
  // other language bindings, if APTIMA runtime wants to help addons do
  // on_xxx_done action at the end of onXxx, APTIMA runtime needs to provide sync
  // API to addons, such that onXxxSync().

  /**
   * @brief on_configure () must be the first public interface function of an
   * extension to be called.
   *
   * @note Extension can _not_ interact with other extensions (ex: send_cmd)
   * in its on_configure() stage.
   */
  axis_extension_on_configure_func_t on_configure;

  /**
   * @brief Extension can initialize itself in its on_init(). After on_init() is
   * completed, the APTIMA runtime will think that the extension can start to
   * respond to commands/data/audio-frames/video-frames.
   *
   * @note Extension can _not_ interact with other extensions (ex: send_cmd)
   * in its on_init() stage.
   */
  axis_extension_on_init_func_t on_init;

  /**
   * @brief on_start() is _not_ always be called before on_cmd(). on_start() can
   * be seen as when a graph is started, it will trigger some operations of the
   * extension. And at the same time, the on_start() of other extensions will
   * also cause the execution of the on_cmd() of the current extension.
   *
   * @note Extension can start to interact with other extensions (ex: send_cmd)
   * in its on_start() stage.
   */
  axis_extension_on_start_func_t on_start;

  /**
   * @note Extension can _still_ interact with other extensions (ex: send_cmd)
   * in its on_stop() stage.
   */
  axis_extension_on_stop_func_t on_stop;

  /**
   * @note Extension can _not_ interact with other extensions (ex: send_cmd) in
   * its on_deinit() stage.
   */
  axis_extension_on_deinit_func_t on_deinit;

  axis_extension_on_cmd_func_t on_cmd;
  axis_extension_on_data_func_t on_data;
  axis_extension_on_audio_frame_func_t on_audio_frame;
  axis_extension_on_video_frame_func_t on_video_frame;
  // @}

  axis_addon_host_t *addon_host;
  axis_string_t name;

  axis_env_t *axis_env;

  axis_extension_thread_t *extension_thread;
  axis_hashhandle_t hh_in_extension_store;

  axis_extension_context_t *extension_context;
  axis_extension_info_t *extension_info;

  axis_value_t manifest;
  axis_value_t property;

  axis_schema_store_t schema_store;

  axis_metadata_info_t *manifest_info;
  axis_metadata_info_t *property_info;

  // @{
  // The following 'pending_msgs' is used to keep the received messages before
  // the extension is started completely.
  //
  // The 'is_started' flag is used to indicate whether on_start_done() has been
  // called. If not, the received messages will be kept in the 'pending_msgs'.
  // Once the on_start_done() is called, the messages in the 'pending_msgs' will
  // be handled.
  //
  // As an exception, the 'cmd result' will be handled normally even if the
  // 'is_started' flag is false.
  //
  // Note that the 'pending_msgs' and 'is_started' flag can only be accessed and
  // modified in the extension thread except during the initialization and
  // deinitialization.
  axis_list_t pending_msgs;
  // @}

  axis_path_table_t *path_table;

  // @{
  // The following 'path_timers' is a list of axis_timer_t, each of which is used
  // to check whether paths in the path table are expired and remove them from
  // the path table. The size of the 'path_timers' could be zero, one (timer
  // used to handle the in_path __or__ out_path) or two (timers used to handle
  // in_path __and__ out_path).
  axis_list_t path_timers;

  // This field is used to store the timeout duration of the in_path and
  // out_path.
  axis_path_timeout_info path_timeout_info;
  // @}

  // Records the number of occurrences of the error code
  // `axis_ERRNO_MSG_NOT_CONNECTED` for each message name when sending output
  // messages.
  axis_hashtable_t msg_not_connected_count_map;

  void *user_data;
};

axis_RUNTIME_PRIVATE_API bool
axis_extension_determine_and_merge_all_interface_dest_extension(
    axis_extension_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_on_init(axis_env_t *axis_env);

axis_RUNTIME_PRIVATE_API void axis_extension_on_start(axis_extension_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_on_stop(axis_extension_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_on_deinit(axis_extension_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_on_cmd(axis_extension_t *self,
                                                  axis_shared_ptr_t *msg);

axis_RUNTIME_PRIVATE_API void axis_extension_on_data(axis_extension_t *self,
                                                   axis_shared_ptr_t *msg);

axis_RUNTIME_PRIVATE_API void axis_extension_on_video_frame(
    axis_extension_t *self, axis_shared_ptr_t *msg);

axis_RUNTIME_PRIVATE_API void axis_extension_on_audio_frame(
    axis_extension_t *self, axis_shared_ptr_t *msg);

axis_RUNTIME_PRIVATE_API void axis_extension_load_metadata(axis_extension_t *self);

axis_RUNTIME_PRIVATE_API void axis_extension_set_addon(
    axis_extension_t *self, axis_addon_host_t *addon_host);

axis_RUNTIME_PRIVATE_API axis_path_in_t *
axis_extension_get_cmd_return_path_from_itself(axis_extension_t *self,
                                              const char *cmd_id);

axis_RUNTIME_PRIVATE_API bool axis_extension_dispatch_msg(
    axis_extension_t *extension, axis_shared_ptr_t *msg, axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_runloop_t *axis_extension_get_attached_runloop(
    axis_extension_t *self);

axis_RUNTIME_PRIVATE_API const char *axis_extension_get_name(
    axis_extension_t *self, bool check_thread);

axis_RUNTIME_API axis_addon_host_t *axis_extension_get_addon(
    axis_extension_t *self);

axis_RUNTIME_PRIVATE_API bool axis_extension_validate_msg_schema(
    axis_extension_t *self, axis_shared_ptr_t *msg, bool is_msg_out,
    axis_error_t *err);

axis_RUNTIME_PRIVATE_API axis_extension_t *axis_extension_from_smart_ptr(
    axis_smart_ptr_t *extension_smart_ptr);

axis_RUNTIME_API void axis_extension_set_me_in_target_lang(
    axis_extension_t *self, void *me_in_target_lang);
