//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/extension/extension_cb_default.h"

#include "ten_utils/macro/check.h"
#include "ten_runtime/common/status_code.h"
#include "ten_runtime/msg/cmd_result/cmd_result.h"
#include "ten_runtime/ten_env/internal/on_xxx_done.h"
#include "ten_runtime/ten_env/internal/return.h"
#include "ten_runtime/ten_env/internal/send.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/macro/mark.h"

void ten_extension_on_init_default(ten_extension_t *self, ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(ten_env, "Should not happen.");

  ten_env_on_init_done(ten_env, NULL);
}

void ten_extension_on_start_default(ten_extension_t *self, ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(ten_env, "Should not happen.");

  ten_env_on_start_done(ten_env, NULL);
}

void ten_extension_on_stop_default(ten_extension_t *self, ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true) && ten_env,
             "Should not happen.");

  ten_env_on_stop_done(ten_env, NULL);
}

void ten_extension_on_deinit_default(ten_extension_t *self,
                                     ten_env_t *ten_env) {
  TEN_ASSERT(self && ten_extension_check_integrity(self, true) && ten_env,
             "Should not happen.");

  ten_env_on_deinit_done(ten_env, NULL);
}

void ten_extension_on_cmd_default(TEN_UNUSED ten_extension_t *self,
                                  TEN_UNUSED ten_env_t *ten_env,
                                  TEN_UNUSED ten_shared_ptr_t *cmd) {
  TEN_ASSERT(
      self && ten_extension_check_integrity(self, true) && ten_env && cmd,
      "Should not happen.");

  // The default behavior of 'on_cmd' is to _not_ forward this command out, and
  // return an 'OK' result to the previous stage.
  ten_shared_ptr_t *cmd_result =
      ten_cmd_result_create_from_cmd(TEN_STATUS_CODE_OK, cmd);
  ten_env_return_result(ten_env, cmd_result, cmd, NULL);
  ten_shared_ptr_destroy(cmd_result);
}

void ten_extension_on_data_default(TEN_UNUSED ten_extension_t *self,
                                   TEN_UNUSED ten_env_t *ten_env,
                                   ten_shared_ptr_t *data) {
  TEN_ASSERT(
      self && ten_extension_check_integrity(self, true) && ten_env && data,
      "Should not happen.");
  // Bypass the data.
  ten_env_send_data(ten_env, data, NULL);
}

void ten_extension_on_audio_frame_default(TEN_UNUSED ten_extension_t *self,
                                          TEN_UNUSED ten_env_t *ten_env,
                                          ten_shared_ptr_t *frame) {
  TEN_ASSERT(
      self && ten_extension_check_integrity(self, true) && ten_env && frame,
      "Should not happen.");
  // Bypass the audio frame.
  ten_env_send_audio_frame(ten_env, frame, NULL);
}

void ten_extension_on_video_frame_default(ten_extension_t *self,
                                          ten_env_t *ten_env,
                                          ten_shared_ptr_t *frame) {
  TEN_ASSERT(
      self && ten_extension_check_integrity(self, true) && ten_env && frame,
      "Should not happen.");
  // Bypass the video frame.
  ten_env_send_video_frame(ten_env, frame, NULL);
}
