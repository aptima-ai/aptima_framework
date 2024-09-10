//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/binding/common.h"
#include "include_internal/ten_runtime/extension/msg_dest_info/all_msg_type_dest_info.h"
#include "include_internal/ten_runtime/extension/path_timer.h"
#include "include_internal/ten_runtime/metadata/metadata_info.h"
#include "include_internal/ten_runtime/schema_store/store.h"
#include "ten_runtime/binding/common.h"
#include "ten_runtime/extension/extension.h"
#include "ten_utils/container/hash_handle.h"
#include "ten_utils/container/list.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/value/value.h"

#define TEN_EXTENSION_SIGNATURE 0xE1627776E09A723CU
#define TEN_EXTENSION_UNIQUE_NAME_IN_GRAPH_PATTERN "%s::%s"

typedef struct ten_env_t ten_env_t;
typedef struct ten_extension_t ten_extension_t;
typedef struct ten_audio_frame_t ten_audio_frame_t;
typedef struct ten_video_frame_t ten_video_frame_t;
typedef struct ten_extension_info_t ten_extension_info_t;
typedef struct ten_extension_context_t ten_extension_context_t;
typedef struct ten_extension_group_t ten_extension_group_t;
typedef struct ten_extension_thread_t ten_extension_thread_t;
typedef struct ten_addon_host_t ten_addon_host_t;
typedef struct ten_path_table_t ten_path_table_t;
typedef struct ten_path_in_t ten_path_in_t;
typedef struct ten_timer_t ten_timer_t;

// The relationship between several lifecycle stages and their connection to
// sending messages:
//
// - on_init ~ on_init_done: Handles its own initialization; cannot send or
//   receive messages.
//
// [ After everyone has completed on_init_done, they will collectively move
//   into on_start ]
//
// - on_start ~ on_start_done: Can send messages and receive the results
//   of sent messages, but cannot receive other messages. Since properties are
//   initialized within on_start, you can perform initialization actions that
//   depend on these properties being set up. However, as it's still in the
//   initializing phase, it won't receive messages initiated by others, avoiding
//   the need for various checks. You can actively send messages out, though.
//
// - After on_start_done ~ on_stop_done: Normal sending and receiving of all
//   messages and results.
//
// [ After everyone has completed on_stop_done, they will collectively move into
//   on_deinit. ]
//
// - on_deinit ~ on_deinit_done: Handles its own de-initialization; cannot send
//   or receive messages.
typedef enum TEN_EXTENSION_STATE {
  TEN_EXTENSION_STATE_INIT,
  TEN_EXTENSION_STATE_INITED,   // on_init_done() is completed.
  TEN_EXTENSION_STATE_STARTED,  // on_start_done() is completed.
  TEN_EXTENSION_STATE_CLOSING,  // on_stop_done() is completed and could proceed
                                // to be closed.
  TEN_EXTENSION_STATE_DEINITING,  // on_deinit() is started.
  TEN_EXTENSION_STATE_DEINITED,   // on_deinit_done() is called.
} TEN_EXTENSION_STATE;

struct ten_extension_t {
  ten_binding_handle_t binding_handle;

  ten_signature_t signature;
  ten_sanitizer_thread_check_t thread_check;

  TEN_EXTENSION_STATE state;

  // @{
  // Public interface.
  //
  // These public APIs are all async behaviors, that is to say, addon needs to
  // actively call on_xxx_done function to notify the TEN runtime that it is
  // done. But in some language bindings (such as JavaScript), because of
  // language-level asynchronous support (such as async/await), TEN runtime can
  // only provide async syntax API to addons (such as `async function onCmd`).
  // Addons can write async codes or sync codes in such async syntax API; but in
  // other language bindings, if TEN runtime wants to help addons do
  // on_xxx_done action at the end of onXxx, TEN runtime needs to provide sync
  // API to addons, such that onXxxSync().

  /**
   * @brief on_init () must be the first public interface function of an
   * extension to be called. Therefore, extension can initialize itself in its
   * on_init(). After on_init() is completed, the TEN runtime will think that
   * the extension can start to respond to
   * commands/data/audio-frames/video-frames.
   *
   * @note Extension can _not_ interact with other extensions (ex: send_cmd) in
   * its on_init() stage.
   */
  ten_extension_on_init_func_t on_init;

  /**
   * @brief on_start() is _not_ always be called before on_cmd(). on_start() can
   * be seen as when a graph is started, it will trigger some operations of the
   * extension. And at the same time, the on_start() of other extensions will
   * also cause the execution of the on_cmd() of the current extension.
   *
   * @note Extension can start to interact with other extensions (ex: send_cmd)
   * in its on_start() stage.
   */
  ten_extension_on_start_func_t on_start;

  /**
   * @note Extension can _still_ interact with other extensions (ex: send_cmd)
   * in its on_stop() stage.
   */
  ten_extension_on_stop_func_t on_stop;

  /**
   * @note Extension can _not_ interact with other extensions (ex: send_cmd) in
   * its on_deinit() stage.
   */
  ten_extension_on_deinit_func_t on_deinit;

  ten_extension_on_cmd_func_t on_cmd;
  ten_extension_on_data_func_t on_data;
  ten_extension_on_audio_frame_func_t on_audio_frame;
  ten_extension_on_video_frame_func_t on_video_frame;
  // @}

  ten_addon_host_t *addon_host;
  ten_string_t name;

  // The extension name is unique in the extension group to which it belongs,
  // and may not be unique in the graph to which it belongs. But in some
  // contexts, a unique name in a graph is needed. The pattern of the unique
  // extension name in a graph is '${extension_group_name}::${extension_name}'.
  ten_string_t unique_name_in_graph;

  ten_string_t base_dir;

  ten_env_t *ten_env;

  ten_extension_thread_t *extension_thread;
  ten_hashhandle_t hh_in_extension_thread_extension_store;

  ten_extension_context_t *extension_context;
  ten_hashhandle_t hh_in_extension_context_extension_store;

  // The current situation is if an extension is generated by an
  // extension_group addon and the extension is not an addon, then this
  // extension does not have extension_info. In the future, for this kind of
  // extension, we can dynamically generate its extension_info after its
  // `on_init_done`.
  ten_extension_info_t *extension_info;

  ten_all_msg_type_dest_runtime_info_t msg_dest_runtime_info;

  ten_value_t manifest;
  ten_value_t property;

  ten_schema_store_t schema_store;

  ten_metadata_info_t *manifest_info;
  ten_metadata_info_t *property_info;

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
  ten_list_t pending_msgs;
  // @}

  ten_path_table_t *path_table;

  // @{
  // The following 'path_timers' is a list of ten_timer_t, each of which is used
  // to check whether paths in the path table are expired and remove them from
  // the path table. The size of the 'path_timers' could be zero, one (timer
  // used to handle the in_path __or__ out_path) or two (timers used to handle
  // in_path __and__ out_path).
  ten_list_t path_timers;

  // This field is used to store the timeout duration of the in_path and
  // out_path.
  ten_path_timeout_info path_timeout_info;
  // @}
};

TEN_RUNTIME_PRIVATE_API void ten_extension_set_state(ten_extension_t *self,
                                                     TEN_EXTENSION_STATE state);

TEN_RUNTIME_PRIVATE_API void ten_extension_determine_all_dest_extension(
    ten_extension_t *self, ten_extension_context_t *extension_context);

TEN_RUNTIME_PRIVATE_API bool
ten_extension_determine_and_merge_all_interface_dest_extension(
    ten_extension_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_link_its_ten_to_extension_context(
    ten_extension_t *self, ten_extension_context_t *extension_context);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_start(ten_extension_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_stop(ten_extension_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_deinit(ten_extension_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_cmd(ten_extension_t *self,
                                                  ten_shared_ptr_t *msg);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_data(ten_extension_t *self,
                                                   ten_shared_ptr_t *msg);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_video_frame(
    ten_extension_t *self, ten_shared_ptr_t *msg);

TEN_RUNTIME_PRIVATE_API void ten_extension_on_audio_frame(
    ten_extension_t *self, ten_shared_ptr_t *msg);

TEN_RUNTIME_PRIVATE_API void ten_extension_load_metadata(ten_extension_t *self);

TEN_RUNTIME_PRIVATE_API void ten_extension_set_addon(
    ten_extension_t *self, ten_addon_host_t *addon_host);

TEN_RUNTIME_PRIVATE_API ten_path_in_t *
ten_extension_get_cmd_return_path_from_itself(ten_extension_t *self,
                                              const char *cmd_id);

TEN_RUNTIME_PRIVATE_API void ten_extension_set_unique_name_in_graph(
    ten_extension_t *self);

TEN_RUNTIME_PRIVATE_API bool ten_extension_handle_out_msg(
    ten_extension_t *extension, ten_shared_ptr_t *msg, ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_runloop_t *ten_extension_get_attached_runloop(
    ten_extension_t *self);

TEN_RUNTIME_PRIVATE_API const char *ten_extension_get_name(
    ten_extension_t *self);

TEN_RUNTIME_PRIVATE_API ten_string_t *ten_extension_get_base_dir(
    ten_extension_t *self);

TEN_RUNTIME_API ten_addon_host_t *ten_extension_get_addon(
    ten_extension_t *self);

TEN_RUNTIME_PRIVATE_API bool ten_extension_validate_msg_schema(
    ten_extension_t *self, ten_shared_ptr_t *msg, bool is_msg_out,
    ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_extension_t *ten_extension_from_smart_ptr(
    ten_smart_ptr_t *extension_smart_ptr);

TEN_RUNTIME_API void ten_extension_set_me_in_target_lang(
    ten_extension_t *self, void *me_in_target_lang);

TEN_RUNTIME_API void ten_extension_direct_all_msg_to_another_extension(
    ten_extension_t *self, ten_extension_t *other);
