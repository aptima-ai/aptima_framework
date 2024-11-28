//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <memory>

#include "ten_runtime/binding/common.h"
#include "ten_runtime/binding/cpp/detail/msg/audio_frame.h"
#include "ten_runtime/binding/cpp/detail/msg/cmd/cmd.h"
#include "ten_runtime/binding/cpp/detail/msg/cmd_result.h"
#include "ten_runtime/binding/cpp/detail/msg/data.h"
#include "ten_runtime/binding/cpp/detail/msg/video_frame.h"
#include "ten_runtime/test/env_tester.h"
#include "ten_utils/lang/cpp/lib/error.h"

using ten_extension_tester_t = struct ten_extension_tester_t;
using ten_env_tester_t = struct ten_env_tester_t;

namespace ten {

class ten_env_tester_t;
class extension_tester_t;

using ten_env_tester_send_cmd_result_handler_func_t = std::function<void(
    ten_env_tester_t &, std::unique_ptr<cmd_result_t>, error_t *)>;

class ten_env_tester_t {
 public:
  // @{
  ten_env_tester_t(const ten_env_tester_t &) = delete;
  ten_env_tester_t(ten_env_tester_t &&) = delete;
  ten_env_tester_t &operator=(const ten_env_tester_t &) = delete;
  ten_env_tester_t &operator=(const ten_env_tester_t &&) = delete;
  // @}};

  bool on_start_done(error_t *err = nullptr) {
    TEN_ASSERT(c_ten_env_tester, "Should not happen.");
    return ten_env_tester_on_start_done(
        c_ten_env_tester,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  bool send_cmd(
      std::unique_ptr<cmd_t> &&cmd,
      ten_env_tester_send_cmd_result_handler_func_t &&result_handler = nullptr,
      error_t *err = nullptr) {
    TEN_ASSERT(c_ten_env_tester, "Should not happen.");

    bool rc = false;

    if (!cmd) {
      TEN_ASSERT(0, "Invalid argument.");
      return rc;
    }

    if (result_handler == nullptr) {
      rc = ten_env_tester_send_cmd(
          c_ten_env_tester, cmd->get_underlying_msg(), nullptr, nullptr,
          err != nullptr ? err->get_internal_representation() : nullptr);
    } else {
      auto *result_handler_ptr =
          new ten_env_tester_send_cmd_result_handler_func_t(
              std::move(result_handler));

      rc = ten_env_tester_send_cmd(
          c_ten_env_tester, cmd->get_underlying_msg(), proxy_handle_result,
          result_handler_ptr,
          err != nullptr ? err->get_internal_representation() : nullptr);
      if (!rc) {
        delete result_handler_ptr;
      }
    }

    if (rc) {
      // Only when the cmd has been sent successfully, we should give back the
      // ownership of the cmd to the TEN runtime.
      auto *cpp_cmd_ptr = cmd.release();
      delete cpp_cmd_ptr;
    } else {
      TEN_LOGE("Failed to send_cmd: %s", cmd->get_name());
    }

    return rc;
  }

  bool send_data(std::unique_ptr<data_t> &&data, error_t *err = nullptr) {
    TEN_ASSERT(c_ten_env_tester, "Should not happen.");

    bool rc = false;

    if (!data) {
      TEN_ASSERT(0, "Invalid argument.");
      return rc;
    }

    rc = ten_env_tester_send_data(
        c_ten_env_tester, data->get_underlying_msg(),
        err != nullptr ? err->get_internal_representation() : nullptr);

    if (rc) {
      // Only when the data has been sent successfully, we should give back the
      // ownership of the data to the TEN runtime.
      auto *cpp_data_ptr = data.release();
      delete cpp_data_ptr;
    } else {
      TEN_LOGE("Failed to send_data: %s", data->get_name());
    }

    return rc;
  }

  bool send_audio_frame(std::unique_ptr<audio_frame_t> &&audio_frame,
                        error_t *err = nullptr) {
    TEN_ASSERT(c_ten_env_tester, "Should not happen.");

    bool rc = false;

    if (!audio_frame) {
      TEN_ASSERT(0, "Invalid argument.");
      return rc;
    }

    rc = ten_env_tester_send_audio_frame(
        c_ten_env_tester, audio_frame->get_underlying_msg(),
        err != nullptr ? err->get_internal_representation() : nullptr);

    if (rc) {
      // Only when the audio_frame has been sent successfully, we should give
      // back the ownership of the audio_frame to the TEN runtime.
      auto *cpp_audio_frame_ptr = audio_frame.release();
      delete cpp_audio_frame_ptr;
    } else {
      TEN_LOGE("Failed to send_audio_frame: %s", audio_frame->get_name());
    }

    return rc;
  }

  bool send_video_frame(std::unique_ptr<video_frame_t> &&video_frame,
                        error_t *err = nullptr) {
    TEN_ASSERT(c_ten_env_tester, "Should not happen.");

    bool rc = false;

    if (!video_frame) {
      TEN_ASSERT(0, "Invalid argument.");
      return rc;
    }

    rc = ten_env_tester_send_video_frame(
        c_ten_env_tester, video_frame->get_underlying_msg(),
        err != nullptr ? err->get_internal_representation() : nullptr);

    if (rc) {
      // Only when the video_frame has been sent successfully, we should give
      // back the ownership of the video_frame to the TEN runtime.
      auto *cpp_video_frame_ptr = video_frame.release();
      delete cpp_video_frame_ptr;
    } else {
      TEN_LOGE("Failed to send_video_frame: %s", video_frame->get_name());
    }

    return rc;
  }

  bool stop_test(error_t *err = nullptr) {
    TEN_ASSERT(c_ten_env_tester, "Should not happen.");
    return ten_env_tester_stop_test(
        c_ten_env_tester,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

 private:
  friend extension_tester_t;
  friend class ten_env_tester_proxy_t;

  ::ten_env_tester_t *c_ten_env_tester;

  virtual ~ten_env_tester_t() {
    TEN_ASSERT(c_ten_env_tester, "Should not happen.");
  }

  explicit ten_env_tester_t(::ten_env_tester_t *c_ten_env_tester)
      : c_ten_env_tester(c_ten_env_tester) {
    TEN_ASSERT(c_ten_env_tester, "Should not happen.");

    ten_binding_handle_set_me_in_target_lang(
        reinterpret_cast<ten_binding_handle_t *>(c_ten_env_tester),
        static_cast<void *>(this));
  }

  static void proxy_handle_result(::ten_env_tester_t *c_ten_env_tester,
                                  ten_shared_ptr_t *c_cmd_result, void *cb_data,
                                  ten_error_t *err) {
    auto *result_handler =
        static_cast<ten_env_tester_send_cmd_result_handler_func_t *>(cb_data);
    auto *cpp_ten_env_tester = static_cast<ten_env_tester_t *>(
        ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(c_ten_env_tester)));

    std::unique_ptr<cmd_result_t> cmd_result = nullptr;

    if (c_cmd_result != nullptr) {
      cmd_result = cmd_result_t::create(
          // Clone a C shared_ptr to be owned by the C++ instance.
          ten_shared_ptr_clone(c_cmd_result));
    }

    if (err != nullptr) {
      error_t cpp_err(err, false);
      (*result_handler)(*cpp_ten_env_tester, std::move(cmd_result), &cpp_err);
    } else {
      (*result_handler)(*cpp_ten_env_tester, std::move(cmd_result), nullptr);
    }

    if (ten_cmd_result_is_final(c_cmd_result, nullptr)) {
      // Only when is_final is true should the result handler be cleared.
      // Otherwise, since more result handlers are expected, the result
      // handler should not be cleared.
      delete result_handler;
    }
  }
};

}  // namespace ten
