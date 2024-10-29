//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <functional>
#include <memory>

#include "ten_runtime/binding/common.h"
#include "ten_runtime/binding/cpp/internal/msg/audio_frame.h"
#include "ten_runtime/binding/cpp/internal/msg/cmd/cmd.h"
#include "ten_runtime/binding/cpp/internal/msg/cmd_result.h"
#include "ten_runtime/binding/cpp/internal/msg/data.h"
#include "ten_runtime/binding/cpp/internal/msg/video_frame.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/ten.h"
#include "ten_runtime/ten_env/internal/metadata.h"
#include "ten_runtime/ten_env/internal/on_xxx_done.h"
#include "ten_runtime/ten_env/internal/return.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lang/cpp/lib/error.h"
#include "ten_utils/lang/cpp/lib/value.h"
#include "ten_utils/lib/buf.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_json.h"

using ten_env_t = struct ten_env_t;

namespace ten {

class app_t;
class extension_t;
class extension_group_t;
class addon_t;
class extension_group_addon_t;
class extension_addon_t;
class ten_env_t;
class ten_env_proxy_t;
class ten_env_internal_accessor_t;

using result_handler_func_t =
    std::function<void(ten_env_t &, std::unique_ptr<cmd_result_t>)>;

using addon_create_extension_async_cb_t =
    std::function<void(ten_env_t &, ten::extension_t &)>;

using addon_destroy_extension_async_cb_t = std::function<void(ten_env_t &)>;

using set_property_async_cb_t =
    std::function<void(ten_env_t &, bool, error_t *err)>;

using get_property_async_cb_t =
    std::function<void(ten_env_t &, ten_value_t *, error_t *err)>;

class ten_env_t {
 public:
  // @{
  ten_env_t(const ten_env_t &) = delete;
  ten_env_t(ten_env_t &&) = delete;
  ten_env_t &operator=(const ten_env_t &) = delete;
  ten_env_t &operator=(const ten_env_t &&) = delete;
  // @}

  bool send_cmd(std::unique_ptr<cmd_t> &&cmd,
                result_handler_func_t &&result_handler, error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    bool rc = false;

    if (!cmd) {
      TEN_ASSERT(0, "Invalid argument.");
      return rc;
    }

    if (result_handler == nullptr) {
      rc = ten_env_send_cmd(
          c_ten_env, cmd->get_underlying_msg(), nullptr, nullptr,
          err != nullptr ? err->get_internal_representation() : nullptr);
    } else {
      auto *result_handler_ptr =
          new result_handler_func_t(std::move(result_handler));

      rc = ten_env_send_cmd(
          c_ten_env, cmd->get_underlying_msg(), proxy_handle_result,
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

  bool send_cmd(std::unique_ptr<cmd_t> &&cmd, error_t *err) {
    return send_cmd(std::move(cmd), nullptr, err);
  }

  bool send_cmd(std::unique_ptr<cmd_t> &&cmd,
                result_handler_func_t &&result_handler) {
    return send_cmd(std::move(cmd), std::move(result_handler), nullptr);
  }

  bool send_cmd(std::unique_ptr<cmd_t> &&cmd) {
    return send_cmd(std::move(cmd), nullptr, nullptr);
  }

  bool send_json(const char *json_str, result_handler_func_t &&result_handler,
                 error_t *err)
      __attribute__((warning("This method may access the '_ten' field. Use "
                             "caution if '_ten' is provided."))) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    if (json_str == nullptr) {
      TEN_ASSERT(0, "Invalid argument.");
      return false;
    }

    ten_json_t *c_json = ten_json_from_string(
        json_str,
        err != nullptr ? err->get_internal_representation() : nullptr);
    if (c_json == nullptr) {
      return false;
    }

    bool rc = false;

    if (result_handler == nullptr) {
      rc = ten_env_send_json(
          c_ten_env, c_json, nullptr, nullptr,
          err != nullptr ? err->get_internal_representation() : nullptr);
    } else {
      auto *result_handler_ptr =
          new result_handler_func_t(std::move(result_handler));

      rc = ten_env_send_json(
          c_ten_env, c_json, proxy_handle_result, result_handler_ptr,
          err != nullptr ? err->get_internal_representation() : nullptr);

      if (!rc) {
        delete result_handler_ptr;
      }
    }

    ten_json_destroy(c_json);

    if (!rc) {
      TEN_LOGE("Failed to send_json: %s", json_str);
    }

    return rc;
  }

  bool send_json(const char *json_str, result_handler_func_t &&result_handler) {
    return send_json(json_str, std::move(result_handler), nullptr);
  }

  bool send_json(const char *json_str, error_t *err) {
    return send_json(json_str, nullptr, err);
  }

  bool send_json(const char *json_str) {
    return send_json(json_str, nullptr, nullptr);
  }

  bool send_data(std::unique_ptr<data_t> &&data, error_t *err) {
    TEN_ASSERT(c_ten_env && data, "Should not happen.");

    if (!data) {
      TEN_ASSERT(0, "Invalid argument.");
      return false;
    }

    if (data->get_underlying_msg() == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(),
                      TEN_ERRNO_INVALID_ARGUMENT, "Invalid data.");
      }
      return false;
    }

    auto rc = ten_env_send_data(
        c_ten_env, data->get_underlying_msg(),
        err != nullptr ? err->get_internal_representation() : nullptr);
    if (rc) {
      // Only when the data has been sent successfully, we should give back
      // the ownership of the data message to the TEN runtime.
      auto *cpp_data_ptr = data.release();
      delete cpp_data_ptr;
    }

    return rc;
  }

  bool send_data(std::unique_ptr<data_t> &&data) {
    return send_data(std::move(data), nullptr);
  }

  bool send_video_frame(std::unique_ptr<video_frame_t> &&frame, error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    if (!frame) {
      TEN_ASSERT(0, "Invalid argument.");
      return false;
    }

    auto rc = ten_env_send_video_frame(
        c_ten_env, frame->get_underlying_msg(),
        err != nullptr ? err->get_internal_representation() : nullptr);

    if (rc) {
      // Only when the message has been sent successfully, we should give back
      // the ownership of the message to the TEN runtime.
      auto *cpp_frame_ptr = frame.release();
      delete cpp_frame_ptr;
    }

    return rc;
  }

  bool send_video_frame(std::unique_ptr<video_frame_t> &&frame) {
    return send_video_frame(std::move(frame), nullptr);
  }

  bool send_audio_frame(std::unique_ptr<audio_frame_t> &&frame, error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    if (!frame) {
      TEN_ASSERT(0, "Invalid argument.");
      return false;
    }

    auto rc = ten_env_send_audio_frame(
        c_ten_env, frame->get_underlying_msg(),
        err != nullptr ? err->get_internal_representation() : nullptr);

    if (rc) {
      // Only when the message has been sent successfully, we should give back
      // the ownership of the message to the TEN runtime.
      auto *cpp_frame_ptr = frame.release();
      delete cpp_frame_ptr;
    }

    return rc;
  }

  bool send_audio_frame(std::unique_ptr<audio_frame_t> &&frame) {
    return send_audio_frame(std::move(frame), nullptr);
  }

  // If the 'cmd' has already been a command in the backward path, a extension
  // could use this API to return the 'cmd' further.
  bool return_result_directly(std::unique_ptr<cmd_result_t> &&cmd,
                              error_t *err) {
    if (!cmd) {
      TEN_ASSERT(0, "Invalid argument.");
      return false;
    }

    auto rc = ten_env_return_result_directly(
        c_ten_env, cmd->get_underlying_msg(),
        err != nullptr ? err->get_internal_representation() : nullptr);

    if (rc) {
      // The 'cmd' has been returned, so we should release the ownership of
      // the C msg from the 'cmd'.
      auto *cpp_cmd_ptr = cmd.release();
      delete cpp_cmd_ptr;
    } else {
      TEN_LOGE("Failed to return_result_directly.");
    }

    return rc;
  }

  bool return_result_directly(std::unique_ptr<cmd_result_t> &&cmd) {
    return return_result_directly(std::move(cmd), nullptr);
  }

  bool return_result(std::unique_ptr<cmd_result_t> &&cmd,
                     std::unique_ptr<cmd_t> &&target_cmd, error_t *err) {
    if (!cmd) {
      TEN_ASSERT(0, "Invalid argument.");
      return false;
    }
    if (!target_cmd) {
      TEN_ASSERT(0, "Invalid argument.");
      return false;
    }

    auto rc = ten_env_return_result(
        c_ten_env, cmd->get_underlying_msg(), target_cmd->get_underlying_msg(),
        err != nullptr ? err->get_internal_representation() : nullptr);

    if (rc) {
      if (cmd->is_final()) {
        // Only when is_final is true does the ownership of target_cmd
        // transfer. Otherwise, target_cmd remains with the extension,
        // allowing the extension to return more results.
        auto *cpp_target_cmd_ptr = target_cmd.release();
        delete cpp_target_cmd_ptr;
      }

      auto *cpp_cmd_ptr = cmd.release();
      delete cpp_cmd_ptr;
    } else {
      TEN_LOGE("Failed to return_result for cmd: %s", target_cmd->get_name());
    }

    return rc;
  }

  bool return_result(std::unique_ptr<cmd_result_t> &&cmd,
                     std::unique_ptr<cmd_t> &&target_cmd) {
    return return_result(std::move(cmd), std::move(target_cmd), nullptr);
  }

  bool is_property_exist(const char *path, error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    if ((path == nullptr) || (strlen(path) == 0)) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(),
                      TEN_ERRNO_INVALID_ARGUMENT, "path should not be empty.");
      }
      return false;
    }

    return ten_env_is_property_exist(
        c_ten_env, path,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  bool is_property_exist(const char *path) {
    return is_property_exist(path, nullptr);
  }

  bool init_property_from_json(const char *json_str, error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    if (json_str == nullptr) {
      TEN_ASSERT(0, "Invalid argument.");
      return false;
    }

    return ten_env_init_property_from_json(
        c_ten_env, json_str,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  bool init_property_from_json(const char *json_str) {
    return init_property_from_json(json_str, nullptr);
  }

  std::string get_property_to_json(const char *path, error_t *err) {
    std::string result;

    if ((path == nullptr) || (strlen(path) == 0)) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(),
                      TEN_ERRNO_INVALID_ARGUMENT, "path should not be empty.");
      }
      return result;
    }

    auto *value = peek_property_value(path, err);
    if (value == nullptr) {
      return result;
    }
    ten_json_t *c_json = ten_value_to_json(value);
    if (c_json == nullptr) {
      return result;
    }

    bool must_free = false;
    const char *json_str = ten_json_to_string(c_json, nullptr, &must_free);
    TEN_ASSERT(json_str, "Failed to convert a JSON to a string");

    result = json_str;

    ten_json_destroy(c_json);
    if (must_free) {
      TEN_FREE(json_str);
    }

    return result;
  }

  std::string get_property_to_json(const char *path) {
    return get_property_to_json(path, nullptr);
  }

  bool set_property_from_json(const char *path, const char *json_str,
                              error_t *err) {
    ten_json_t *c_json = ten_json_from_string(
        json_str,
        err != nullptr ? err->get_internal_representation() : nullptr);
    if (c_json == nullptr) {
      return false;
    }

    ten_value_t *value = ten_value_from_json(c_json);
    ten_json_destroy(c_json);

    return set_property_impl(path, value, err);
  }

  bool set_property_from_json(const char *path, const char *json_str) {
    return set_property_from_json(path, json_str, nullptr);
  }

  uint8_t get_property_uint8(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return 0;
    }
    return ten_value_get_uint8(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  uint8_t get_property_uint8(const char *path) {
    return get_property_uint8(path, nullptr);
  }

  uint16_t get_property_uint16(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return 0;
    }
    return ten_value_get_uint16(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  uint16_t get_property_uint16(const char *path) {
    return get_property_uint16(path, nullptr);
  }

  uint32_t get_property_uint32(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return 0;
    }
    return ten_value_get_uint32(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  uint32_t get_property_uint32(const char *path) {
    return get_property_uint32(path, nullptr);
  }

  uint64_t get_property_uint64(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return 0;
    }
    return ten_value_get_uint64(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  uint64_t get_property_uint64(const char *path) {
    return get_property_uint64(path, nullptr);
  }

  int8_t get_property_int8(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return 0;
    }
    return ten_value_get_int8(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  int8_t get_property_int8(const char *path) {
    return get_property_int8(path, nullptr);
  }

  int16_t get_property_int16(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return 0;
    }
    return ten_value_get_int16(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  int16_t get_property_int16(const char *path) {
    return get_property_int16(path, nullptr);
  }

  int32_t get_property_int32(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return 0;
    }
    return ten_value_get_int32(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  int32_t get_property_int32(const char *path) {
    return get_property_int32(path, nullptr);
  }

  int64_t get_property_int64(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return 0;
    }
    return ten_value_get_int64(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  int64_t get_property_int64(const char *path) {
    return get_property_int64(path, nullptr);
  }

  float get_property_float32(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return 0.0F;
    }
    return ten_value_get_float32(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  float get_property_float32(const char *path) {
    return get_property_float32(path, nullptr);
  }

  double get_property_float64(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return 0.0F;
    }
    return ten_value_get_float64(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  double get_property_float64(const char *path) {
    return get_property_float64(path, nullptr);
  }

  std::string get_property_string(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return "";
    }
    return ten_value_peek_raw_str(c_value);
  }

  std::string get_property_string(const char *path) {
    return get_property_string(path, nullptr);
  }

  void *get_property_ptr(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return nullptr;
    }
    return ten_value_get_ptr(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  void *get_property_ptr(const char *path) {
    return get_property_ptr(path, nullptr);
  }

  bool get_property_bool(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return false;
    }

    return ten_value_get_bool(
        c_value, err != nullptr ? err->get_internal_representation() : nullptr);
  }

  bool get_property_bool(const char *path) {
    return get_property_bool(path, nullptr);
  }

  bool get_property_int32_async(
      const char *path,
      std::function<void(ten_env_t &, int32_t, error_t *err)> &&cb,
      error_t *err) {
    // Initialized lambda captures are a C++14 extension, and we use the
    // following trick to achieve the same effect.
    auto cb_copy = std::move(cb);
    return get_property_async_impl(
        path,
        [cb_copy](ten_env_t &ten_env, ten_value_t *value, error_t *err) {
          cb_copy(ten_env,
                  ten_value_get_int32(
                      value, err != nullptr ? err->get_internal_representation()
                                            : nullptr),
                  err);
          return;
        },
        err);
  }

  bool get_property_int32_async(
      const char *path,
      std::function<void(ten_env_t &, int32_t, error_t *err)> &&cb) {
    return get_property_int32_async(path, std::move(cb), nullptr);
  }

  bool get_property_string_async(
      const char *path,
      std::function<void(ten_env_t &, const std::string &, error_t *err)> &&cb,
      error_t *err) {
    // Initialized lambda captures are a C++14 extension, and we use the
    // following trick to achieve the same effect.
    auto cb_copy = std::move(cb);
    return get_property_async_impl(
        path,
        [cb_copy](ten_env_t &ten_env, ten_value_t *value, error_t *err) {
          cb_copy(ten_env, ten_value_peek_raw_str(value), err);
          return;
        },
        err);
  }

  bool get_property_string_async(
      const char *path,
      std::function<void(ten_env_t &, const std::string &, error_t *err)>
          &&cb) {
    return get_property_string_async(path, std::move(cb), nullptr);
  }

  bool set_property(const char *path, int8_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_int8(value), err);
  }

  bool set_property(const char *path, int8_t value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, int16_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_int16(value), err);
  }

  bool set_property(const char *path, int16_t value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, int32_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_int32(value), err);
  }

  bool set_property(const char *path, int32_t value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, int64_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_int64(value), err);
  }

  bool set_property(const char *path, int64_t value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, uint8_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_uint8(value), err);
  }

  bool set_property(const char *path, uint8_t value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, uint16_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_uint16(value), err);
  }

  bool set_property(const char *path, uint16_t value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, uint32_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_uint32(value), err);
  }

  bool set_property(const char *path, uint32_t value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, uint64_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_uint64(value), err);
  }

  bool set_property(const char *path, uint64_t value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, float value, error_t *err) {
    return set_property_impl(path, ten_value_create_float32(value), err);
  }

  bool set_property(const char *path, float value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, double value, error_t *err) {
    return set_property_impl(path, ten_value_create_float64(value), err);
  }

  bool set_property(const char *path, double value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, bool value, error_t *err) {
    return set_property_impl(path, ten_value_create_bool(value), err);
  }

  bool set_property(const char *path, bool value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, void *value, error_t *err) {
    return set_property_impl(
        path, ten_value_create_ptr(value, nullptr, nullptr, nullptr), err);
  }

  bool set_property(const char *path, void *value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, const char *value, error_t *err) {
    return set_property_impl(path, ten_value_create_string(value), err);
  }

  bool set_property(const char *path, const char *value) {
    return set_property(path, value, nullptr);
  }

  // Convenient overloaded function for string type.
  bool set_property(const char *path, const std::string &value, error_t *err) {
    return set_property_impl(path, ten_value_create_string(value.c_str()), err);
  }

  bool set_property(const char *path, const std::string &value) {
    return set_property(path, value, nullptr);
  }

  bool set_property(const char *path, const ten::buf_t &value, error_t *err) {
    ten_buf_t buf =
        TEN_BUF_STATIC_INIT_WITH_DATA_OWNED(value.data(), value.size());
    return set_property_impl(path, ten_value_create_buf_with_move(buf), err);
  }

  bool set_property(const char *path, const ten::buf_t &value) {
    return set_property(path, value, nullptr);
  }

  bool set_property_async(const char *path, int8_t value,
                          set_property_async_cb_t &&cb, error_t *err) {
    return set_property_async_impl(path, ten_value_create_int8(value),
                                   std::move(cb), err);
  }

  bool set_property_async(const char *path, int8_t value,
                          set_property_async_cb_t &&cb) {
    return set_property_async(path, value, std::move(cb), nullptr);
  }

  bool set_property_async(const char *path, int16_t value,
                          set_property_async_cb_t &&cb, error_t *err) {
    return set_property_async_impl(path, ten_value_create_int16(value),
                                   std::move(cb), err);
  }

  bool set_property_async(const char *path, int16_t value,
                          set_property_async_cb_t &&cb) {
    return set_property_async(path, value, std::move(cb), nullptr);
  }

  bool set_property_async(const char *path, int32_t value,
                          set_property_async_cb_t &&cb, error_t *err) {
    return set_property_async_impl(path, ten_value_create_int32(value),
                                   std::move(cb), err);
  }

  bool set_property_async(const char *path, int32_t value,
                          set_property_async_cb_t &&cb) {
    return set_property_async(path, value, std::move(cb), nullptr);
  }

  bool set_property_async(const char *path, int64_t value,
                          set_property_async_cb_t &&cb, error_t *err) {
    return set_property_async_impl(path, ten_value_create_int64(value),
                                   std::move(cb), err);
  }

  bool set_property_async(const char *path, int64_t value,
                          set_property_async_cb_t &&cb) {
    return set_property_async(path, value, std::move(cb), nullptr);
  }

  bool set_property_async(const char *path, uint8_t value,
                          set_property_async_cb_t &&cb, error_t *err) {
    return set_property_async_impl(path, ten_value_create_uint8(value),
                                   std::move(cb), err);
  }

  bool set_property_async(const char *path, uint8_t value,
                          set_property_async_cb_t &&cb) {
    return set_property_async(path, value, std::move(cb), nullptr);
  }

  bool set_property_async(const char *path, uint16_t value,
                          set_property_async_cb_t &&cb, error_t *err) {
    return set_property_async_impl(path, ten_value_create_uint16(value),
                                   std::move(cb), err);
  }

  bool set_property_async(const char *path, uint16_t value,
                          set_property_async_cb_t &&cb) {
    return set_property_async(path, value, std::move(cb), nullptr);
  }

  bool set_property_async(const char *path, uint32_t value,
                          set_property_async_cb_t &&cb, error_t *err) {
    return set_property_async_impl(path, ten_value_create_uint32(value),
                                   std::move(cb), err);
  }

  bool set_property_async(const char *path, uint32_t value,
                          set_property_async_cb_t &&cb) {
    return set_property_async(path, value, std::move(cb), nullptr);
  }

  bool set_property_async(const char *path, uint64_t value,
                          set_property_async_cb_t &&cb, error_t *err) {
    return set_property_async_impl(path, ten_value_create_uint64(value),
                                   std::move(cb), err);
  }

  bool set_property_async(const char *path, uint64_t value,
                          set_property_async_cb_t &&cb) {
    return set_property_async(path, value, std::move(cb), nullptr);
  }

  bool set_property_async(const char *path, const char *value,
                          set_property_async_cb_t &&cb, error_t *err) {
    return set_property_async_impl(path, ten_value_create_string(value),
                                   std::move(cb), err);
  }

  bool set_property_async(const char *path, const char *value,
                          set_property_async_cb_t &&cb) {
    return set_property_async(path, value, std::move(cb), nullptr);
  }

  // Convenient overloaded function for string type.
  bool set_property_async(const char *path, const std::string &value,
                          set_property_async_cb_t &&cb, error_t *err) {
    return set_property_async_impl(path, ten_value_create_string(value.c_str()),
                                   std::move(cb), err);
  }

  bool set_property_async(const char *path, const std::string &value,
                          set_property_async_cb_t &&cb) {
    return set_property_async(path, value, std::move(cb), nullptr);
  }

  bool is_cmd_connected(const char *cmd_name, error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");
    return ten_env_is_cmd_connected(
        c_ten_env, cmd_name,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  bool is_cmd_connected(const char *cmd_name) {
    return is_cmd_connected(cmd_name, nullptr);
  }

  bool addon_create_extension_async(const char *addon_name,
                                    const char *instance_name,
                                    addon_create_extension_async_cb_t &&cb,
                                    error_t *err) {
    if (cb == nullptr) {
      return ten_addon_create_extension_async(
          c_ten_env, addon_name, instance_name, nullptr, nullptr,
          err != nullptr ? err->get_internal_representation() : nullptr);
    } else {
      auto *cb_ptr = new addon_create_extension_async_cb_t(std::move(cb));

      return ten_addon_create_extension_async(
          c_ten_env, addon_name, instance_name,
          proxy_addon_create_extension_async_cb, cb_ptr,
          err != nullptr ? err->get_internal_representation() : nullptr);
    }
  }

  bool addon_create_extension_async(const char *addon_name,
                                    const char *instance_name,
                                    addon_create_extension_async_cb_t &&cb) {
    return addon_create_extension_async(addon_name, instance_name,
                                        std::move(cb), nullptr);
  }

  bool addon_create_extension_async(const char *addon_name,
                                    const char *instance_name) {
    return addon_create_extension_async(addon_name, instance_name, nullptr,
                                        nullptr);
  }

  bool addon_destroy_extension(ten::extension_t *extension, error_t *err);

  bool addon_destroy_extension(ten::extension_t *extension) {
    return addon_destroy_extension(extension, nullptr);
  }

  bool addon_destroy_extension_async(ten::extension_t *extension,
                                     addon_destroy_extension_async_cb_t &&cb,
                                     error_t *err);

  bool addon_destroy_extension_async(ten::extension_t *extension) {
    return addon_destroy_extension_async(extension, nullptr, nullptr);
  }

  bool addon_destroy_extension_async(ten::extension_t *extension,
                                     addon_destroy_extension_async_cb_t &&cb) {
    return addon_destroy_extension_async(extension, std::move(cb), nullptr);
  }

  bool addon_destroy_extension_async(ten::extension_t *extension,
                                     error_t *err) {
    return addon_destroy_extension_async(extension, nullptr, err);
  }

  bool on_configure_done() { return on_configure_done(nullptr); }

  bool on_configure_done(error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    bool rc = ten_env_on_configure_done(
        c_ten_env,
        err != nullptr ? err->get_internal_representation() : nullptr);

    return rc;
  }

  bool on_init_done(error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    bool rc = ten_env_on_init_done(
        c_ten_env,
        err != nullptr ? err->get_internal_representation() : nullptr);

    return rc;
  }

  bool on_init_done() { return on_init_done(nullptr); }

  bool on_deinit_done(error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");
    return ten_env_on_deinit_done(
        c_ten_env,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  bool on_deinit_done() { return on_deinit_done(nullptr); }

  bool on_start_done(error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");
    return ten_env_on_start_done(
        c_ten_env,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  bool on_start_done() { return on_start_done(nullptr); }

  bool on_stop_done(error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");
    return ten_env_on_stop_done(
        c_ten_env,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  bool on_stop_done() { return on_stop_done(nullptr); }

  bool on_create_extensions_done(const std::vector<extension_t *> &extensions,
                                 error_t *err);

  bool on_create_extensions_done(const std::vector<extension_t *> &extensions) {
    return on_create_extensions_done(extensions, nullptr);
  }

  bool on_destroy_extensions_done(error_t *err) {
    return ten_env_on_destroy_extensions_done(
        c_ten_env,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  bool on_destroy_extensions_done() {
    return on_destroy_extensions_done(nullptr);
  }

  bool on_create_instance_done(void *instance, void *context, error_t *err);

  bool on_create_instance_done(void *instance, void *context) {
    return on_create_instance_done(instance, context, nullptr);
  }

  bool on_destroy_instance_done(void *context, error_t *err) {
    bool rc = ten_env_on_destroy_instance_done(
        c_ten_env, context,
        err != nullptr ? err->get_internal_representation() : nullptr);

    return rc;
  }

  bool on_destroy_instance_done(void *context) {
    return on_destroy_instance_done(context, nullptr);
  }

  void *get_attached_target(error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    return ten_binding_handle_get_me_in_target_lang(
        reinterpret_cast<ten_binding_handle_t *>(
            ten_env_get_attached_target(c_ten_env)));
  }

  void *get_attached_target() { return get_attached_target(nullptr); }

#define TEN_ENV_LOG_VERBOSE(ten_env, msg)                                      \
  do {                                                                         \
    (ten_env).log(TEN_LOG_LEVEL_VERBOSE, __func__, __FILE__, __LINE__, (msg)); \
  } while (0)

#define TEN_ENV_LOG_DEBUG(ten_env, msg)                                      \
  do {                                                                       \
    (ten_env).log(TEN_LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, (msg)); \
  } while (0)

#define TEN_ENV_LOG_INFO(ten_env, msg)                                      \
  do {                                                                      \
    (ten_env).log(TEN_LOG_LEVEL_INFO, __func__, __FILE__, __LINE__, (msg)); \
  } while (0)

#define TEN_ENV_LOG_WARN(ten_env, msg)                                      \
  do {                                                                      \
    (ten_env).log(TEN_LOG_LEVEL_WARN, __func__, __FILE__, __LINE__, (msg)); \
  } while (0)

#define TEN_ENV_LOG_ERROR(ten_env, msg)                                      \
  do {                                                                       \
    (ten_env).log(TEN_LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__, (msg)); \
  } while (0)

#define TEN_ENV_LOG_FATAL(ten_env, msg)                                      \
  do {                                                                       \
    (ten_env).log(TEN_LOG_LEVEL_FATAL, __func__, __FILE__, __LINE__, (msg)); \
  } while (0)

#define TEN_ENV_LOG(ten_env, level, msg)                         \
  do {                                                           \
    (ten_env).log((level), __func__, __FILE__, __LINE__, (msg)); \
  } while (0)

  void log(TEN_LOG_LEVEL level, const char *func_name, const char *file_name,
           size_t line_no, const char *msg) {
    TEN_ASSERT(c_ten_env, "Should not happen.");
    ten_env_log(c_ten_env, level, func_name, file_name, line_no, msg);
  }

 private:
  friend class ten_env_proxy_t;
  friend class app_t;
  friend class extension_t;
  friend class extension_group_t;
  friend class addon_t;
  friend class extension_group_addon_t;
  friend class extension_addon_t;
  friend class ten_env_internal_accessor_t;

  ::ten_env_t *c_ten_env;

  explicit ten_env_t(::ten_env_t *c_ten_env) : c_ten_env(c_ten_env) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    ten_binding_handle_set_me_in_target_lang(
        reinterpret_cast<ten_binding_handle_t *>(c_ten_env),
        static_cast<void *>(this));
  }

  ~ten_env_t() { TEN_ASSERT(c_ten_env, "Should not happen."); }

  ::ten_env_t *get_c_ten_env() { return c_ten_env; }

  bool init_manifest_from_json(const char *json_str, error_t *err);

  ten_value_t *peek_property_value(const char *path, error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    return ten_env_peek_property(
        c_ten_env, path,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  /**
   * @note Note the move semantics of @a value. The @a value should not be
   * used after calling this function.
   */
  bool set_property_impl(const char *path, ten_value_t *value, error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    bool rc = ten_env_set_property(
        c_ten_env, path, value,
        err != nullptr ? err->get_internal_representation() : nullptr);

    if (!rc) {
      ten_value_destroy(value);
    }
    return rc;
  }

  /**
   * @note Note the move semantics of @a value. The @a value should not be
   * used after calling this function.
   */
  bool set_property_async_impl(const char *path, ten_value_t *value,
                               set_property_async_cb_t &&cb, error_t *err) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    auto *cb_ptr = new set_property_async_cb_t(cb);

    bool rc = ten_env_set_property_async(
        c_ten_env, path, value, proxy_set_property_callback, cb_ptr,
        err != nullptr ? err->get_internal_representation() : nullptr);

    if (!rc) {
      delete cb_ptr;
    }
    return rc;
  }

  bool get_property_async_impl(const char *path, get_property_async_cb_t &&cb,
                               error_t *err = nullptr) {
    TEN_ASSERT(c_ten_env, "Should not happen.");

    auto *cb_ptr = new get_property_async_cb_t(cb);

    bool rc = ten_env_peek_property_async(
        c_ten_env, path, proxy_get_property_async_from_peek_cb, cb_ptr,
        err != nullptr ? err->get_internal_representation() : nullptr);

    if (!rc) {
      delete cb_ptr;
    }

    return rc;
  }

  static void proxy_handle_result(TEN_UNUSED ten_extension_t *extension,
                                  ::ten_env_t *ten_env,
                                  ten_shared_ptr_t *c_cmd_result,
                                  void *cb_data) {
    auto *result_handler = static_cast<result_handler_func_t *>(cb_data);
    auto *cpp_ten_env =
        static_cast<ten_env_t *>(ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(ten_env)));

    auto cmd_result = cmd_result_t::create(
        // Clone a C shared_ptr to be owned by the C++ instance.
        ten_shared_ptr_clone(c_cmd_result));

    (*result_handler)(*cpp_ten_env, std::move(cmd_result));

    if (ten_cmd_result_is_final(c_cmd_result, nullptr)) {
      // Only when is_final is true should the result handler be cleared.
      // Otherwise, since more result handlers are expected, the result
      // handler should not be cleared.
      delete result_handler;
    }
  }

  static void proxy_addon_create_extension_async_cb(::ten_env_t *ten_env,
                                                    void *instance,
                                                    void *cb_data) {
    auto *addon_create_extension_async_cb =
        static_cast<addon_create_extension_async_cb_t *>(cb_data);
    auto *cpp_ten_env =
        static_cast<ten_env_t *>(ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(ten_env)));

    (*addon_create_extension_async_cb)(
        *cpp_ten_env,
        *static_cast<ten::extension_t *>(
            ten_binding_handle_get_me_in_target_lang(
                reinterpret_cast<ten_binding_handle_t *>(instance))));
    delete addon_create_extension_async_cb;
  }

  static void proxy_addon_destroy_extension_async_cb(::ten_env_t *ten_env,
                                                     void *cb_data) {
    auto *addon_destroy_extension_async_cb =
        static_cast<addon_destroy_extension_async_cb_t *>(cb_data);
    auto *cpp_ten_env =
        static_cast<ten_env_t *>(ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(ten_env)));

    (*addon_destroy_extension_async_cb)(*cpp_ten_env);
    delete addon_destroy_extension_async_cb;
  }

  static void proxy_set_property_callback(::ten_env_t *ten_env, bool res,
                                          void *cb_data, ten_error_t *err) {
    auto *callback = static_cast<set_property_async_cb_t *>(cb_data);
    auto *cpp_ten_env =
        static_cast<ten_env_t *>(ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(ten_env)));

    error_t *cpp_err = nullptr;
    if (err != nullptr) {
      cpp_err = new error_t(err, false);
    }

    (*callback)(*cpp_ten_env, res, cpp_err);

    delete cpp_err;
    delete callback;
  }

  static void proxy_get_property_async_from_peek_cb(::ten_env_t *ten_env,
                                                    ten_value_t *res,
                                                    void *cb_data,
                                                    ten_error_t *err) {
    auto *callback = static_cast<get_property_async_cb_t *>(cb_data);
    auto *cpp_ten_env =
        static_cast<ten_env_t *>(ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(ten_env)));

    error_t *cpp_err = nullptr;
    if (err != nullptr) {
      cpp_err = new error_t(err, false);
    }

    (*callback)(*cpp_ten_env, res, cpp_err);

    delete cpp_err;
    delete callback;
  }
};

}  // namespace ten
