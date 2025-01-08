//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <memory>

#include "axis_runtime/binding/common.h"
#include "axis_runtime/binding/cpp/detail/msg/audio_frame.h"
#include "axis_runtime/binding/cpp/detail/msg/cmd/cmd.h"
#include "axis_runtime/binding/cpp/detail/msg/cmd_result.h"
#include "axis_runtime/binding/cpp/detail/msg/data.h"
#include "axis_runtime/binding/cpp/detail/msg/video_frame.h"
#include "axis_runtime/test/env_tester.h"
#include "axis_utils/lang/cpp/lib/error.h"

using axis_extension_tester_t = struct axis_extension_tester_t;
using axis_env_tester_t = struct axis_env_tester_t;

namespace ten {

class axis_env_tester_t;
class extension_tester_t;

using axis_env_tester_send_cmd_result_handler_func_t = std::function<void(
    axis_env_tester_t &, std::unique_ptr<cmd_result_t>, error_t *)>;

class axis_env_tester_t {
 public:
  // @{
  axis_env_tester_t(const axis_env_tester_t &) = delete;
  axis_env_tester_t(axis_env_tester_t &&) = delete;
  axis_env_tester_t &operator=(const axis_env_tester_t &) = delete;
  axis_env_tester_t &operator=(const axis_env_tester_t &&) = delete;
  // @}};

  bool on_start_done(error_t *err = nullptr) {
    axis_ASSERT(c_axis_env_tester, "Should not happen.");
    return axis_env_tester_on_start_done(
        c_axis_env_tester, err != nullptr ? err->get_c_error() : nullptr);
  }

  bool send_cmd(
      std::unique_ptr<cmd_t> &&cmd,
      axis_env_tester_send_cmd_result_handler_func_t &&result_handler = nullptr,
      error_t *err = nullptr) {
    axis_ASSERT(c_axis_env_tester, "Should not happen.");

    bool rc = false;

    if (!cmd) {
      axis_ASSERT(0, "Invalid argument.");
      return rc;
    }

    if (result_handler == nullptr) {
      rc = axis_env_tester_send_cmd(
          c_axis_env_tester, cmd->get_underlying_msg(), nullptr, nullptr,
          err != nullptr ? err->get_c_error() : nullptr);
    } else {
      auto *result_handler_ptr =
          new axis_env_tester_send_cmd_result_handler_func_t(
              std::move(result_handler));

      rc = axis_env_tester_send_cmd(
          c_axis_env_tester, cmd->get_underlying_msg(), proxy_handle_result,
          result_handler_ptr, err != nullptr ? err->get_c_error() : nullptr);
      if (!rc) {
        delete result_handler_ptr;
      }
    }

    if (rc) {
      // Only when the cmd has been sent successfully, we should give back the
      // ownership of the cmd to the TEN runtime.
      auto *cpp_cmd_ptr = cmd.release();
      delete cpp_cmd_ptr;
    }

    return rc;
  }

  bool send_data(std::unique_ptr<data_t> &&data, error_t *err = nullptr) {
    axis_ASSERT(c_axis_env_tester, "Should not happen.");

    bool rc = false;

    if (!data) {
      axis_ASSERT(0, "Invalid argument.");
      return rc;
    }

    rc = axis_env_tester_send_data(
        c_axis_env_tester, data->get_underlying_msg(), nullptr, nullptr,
        err != nullptr ? err->get_c_error() : nullptr);

    if (rc) {
      // Only when the data has been sent successfully, we should give back the
      // ownership of the data to the TEN runtime.
      auto *cpp_data_ptr = data.release();
      delete cpp_data_ptr;
    }

    return rc;
  }

  bool send_audio_frame(std::unique_ptr<audio_frame_t> &&audio_frame,
                        error_t *err = nullptr) {
    axis_ASSERT(c_axis_env_tester, "Should not happen.");

    bool rc = false;

    if (!audio_frame) {
      axis_ASSERT(0, "Invalid argument.");
      return rc;
    }

    rc = axis_env_tester_send_audio_frame(
        c_axis_env_tester, audio_frame->get_underlying_msg(), nullptr, nullptr,
        err != nullptr ? err->get_c_error() : nullptr);

    if (rc) {
      // Only when the audio_frame has been sent successfully, we should give
      // back the ownership of the audio_frame to the TEN runtime.
      auto *cpp_audio_frame_ptr = audio_frame.release();
      delete cpp_audio_frame_ptr;
    }

    return rc;
  }

  bool send_video_frame(std::unique_ptr<video_frame_t> &&video_frame,
                        error_t *err = nullptr) {
    axis_ASSERT(c_axis_env_tester, "Should not happen.");

    bool rc = false;

    if (!video_frame) {
      axis_ASSERT(0, "Invalid argument.");
      return rc;
    }

    rc = axis_env_tester_send_video_frame(
        c_axis_env_tester, video_frame->get_underlying_msg(), nullptr, nullptr,
        err != nullptr ? err->get_c_error() : nullptr);

    if (rc) {
      // Only when the video_frame has been sent successfully, we should give
      // back the ownership of the video_frame to the TEN runtime.
      auto *cpp_video_frame_ptr = video_frame.release();
      delete cpp_video_frame_ptr;
    }

    return rc;
  }

  bool return_result(std::unique_ptr<cmd_result_t> &&cmd_result,
                     std::unique_ptr<cmd_t> &&target_cmd,
                     error_t *err = nullptr) {
    axis_ASSERT(c_axis_env_tester, "Should not happen.");

    bool rc = false;

    if (!cmd_result || !target_cmd) {
      axis_ASSERT(0, "Invalid argument.");
      return rc;
    }

    rc = axis_env_tester_return_result(
        c_axis_env_tester, cmd_result->get_underlying_msg(),
        target_cmd->get_underlying_msg(), nullptr, nullptr,
        err != nullptr ? err->get_c_error() : nullptr);

    if (rc) {
      // Only when is_final is true does the ownership of target_cmd transfer.
      // Otherwise, target_cmd remains with the extension, allowing the
      // extension to return more results.
      if (cmd_result->is_final()) {
        auto *cpp_target_cmd_ptr = target_cmd.release();
        delete cpp_target_cmd_ptr;
      }

      auto *cpp_cmd_result_ptr = cmd_result.release();
      delete cpp_cmd_result_ptr;
    }

    return rc;
  }

  bool stop_test(error_t *err = nullptr) {
    axis_ASSERT(c_axis_env_tester, "Should not happen.");
    return axis_env_tester_stop_test(
        c_axis_env_tester, err != nullptr ? err->get_c_error() : nullptr);
  }

 private:
  friend extension_tester_t;
  friend class axis_env_tester_proxy_t;

  ::axis_env_tester_t *c_axis_env_tester;

  virtual ~axis_env_tester_t() {
    axis_ASSERT(c_axis_env_tester, "Should not happen.");
  }

  explicit axis_env_tester_t(::axis_env_tester_t *c_axis_env_tester)
      : c_axis_env_tester(c_axis_env_tester) {
    axis_ASSERT(c_axis_env_tester, "Should not happen.");

    axis_binding_handle_set_me_in_target_lang(
        reinterpret_cast<axis_binding_handle_t *>(c_axis_env_tester),
        static_cast<void *>(this));
  }

  static void proxy_handle_result(::axis_env_tester_t *c_axis_env_tester,
                                  axis_shared_ptr_t *c_cmd_result, void *cb_data,
                                  axis_error_t *err) {
    auto *result_handler =
        static_cast<axis_env_tester_send_cmd_result_handler_func_t *>(cb_data);
    auto *cpp_axis_env_tester = static_cast<axis_env_tester_t *>(
        axis_binding_handle_get_me_in_target_lang(
            reinterpret_cast<axis_binding_handle_t *>(c_axis_env_tester)));

    std::unique_ptr<cmd_result_t> cmd_result = nullptr;

    if (c_cmd_result != nullptr) {
      cmd_result = cmd_result_t::create(
          // Clone a C shared_ptr to be owned by the C++ instance.
          axis_shared_ptr_clone(c_cmd_result));
    }

    if (err != nullptr) {
      error_t cpp_err(err, false);
      (*result_handler)(*cpp_axis_env_tester, std::move(cmd_result), &cpp_err);
    } else {
      (*result_handler)(*cpp_axis_env_tester, std::move(cmd_result), nullptr);
    }

    if (axis_cmd_result_is_final(c_cmd_result, nullptr)) {
      // Only when is_final is true should the result handler be cleared.
      // Otherwise, since more result handlers are expected, the result handler
      // should not be cleared.
      delete result_handler;
    }
  }
};

}  // namespace ten
