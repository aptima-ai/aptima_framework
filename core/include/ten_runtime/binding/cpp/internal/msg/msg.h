//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <string>

#include "ten_utils/macro/check.h"
#include "ten_runtime/common/errno.h"
#include "ten_runtime/msg/msg.h"
#include "ten_utils/lang/cpp/lib/error.h"
#include "ten_utils/lang/cpp/lib/value.h"
#include "ten_utils/lib/buf.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_get.h"
#include "ten_utils/value/value_json.h"

namespace ten {

class ten_env_t;
class msg_t;
class cmd_result_t;

namespace {

template <typename T>
struct is_string
    : public std::integral_constant<
          bool,
          std::is_same<char *, typename std::decay<T>::type>::value ||
              std::is_same<const char *, typename std::decay<T>::type>::value> {
};

template <>
struct is_string<std::string> : std::true_type {};

}  // namespace

class msg_t {
 public:
  virtual ~msg_t() { relinquish_underlying_msg(); }

  // @{
  msg_t(msg_t &other) = delete;
  msg_t(msg_t &&other) = delete;
  msg_t &operator=(const msg_t &other) = delete;
  msg_t &operator=(msg_t &&other) noexcept = delete;
  // @}

  explicit operator bool() const { return c_msg_ != nullptr; }

  TEN_MSG_TYPE get_type(error_t *err = nullptr) const {
    if (c_msg_ == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(),
                      TEN_ERRNO_INVALID_ARGUMENT, "Invalid TEN message.");
      }
      return TEN_MSG_TYPE_INVALID;
    }

    return ten_msg_get_type(c_msg_);
  }

  const char *get_name(error_t *err = nullptr) const {
    TEN_ASSERT(c_msg_, "Should not happen.");

    if (c_msg_ == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(), TEN_ERRNO_GENERIC,
                      "Invalid TEN message.");
      }
      return "";
    }

    return ten_msg_get_name(c_msg_);
  }

  bool set_dest(const char *uri, const char *graph,
                const char *extension_group_name, const char *extension_name,
                error_t *err = nullptr) const {
    TEN_ASSERT(c_msg_, "Should not happen.");

    if (c_msg_ == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(),
                      TEN_ERRNO_INVALID_ARGUMENT, "Invalid TEN message.");
      }
      return false;
    }

    return ten_msg_clear_and_set_dest(
        c_msg_, uri, graph, extension_group_name, extension_name, nullptr,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  std::string to_json(error_t *err = nullptr) const {
    TEN_ASSERT(c_msg_, "Should not happen.");

    if (c_msg_ == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(), TEN_ERRNO_GENERIC,
                      "Invalid TEN message.");
      }
      return "";
    }

    ten_json_t *c_json = ten_msg_to_json(
        c_msg_, err != nullptr ? err->get_internal_representation() : nullptr);
    TEN_ASSERT(c_json, "Failed to get json from TEN C message.");
    if (c_json == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(), TEN_ERRNO_GENERIC,
                      "Invalid TEN message.");
      }
      return "";
    }

    bool must_free = false;
    const char *json_str = ten_json_to_string(c_json, nullptr, &must_free);
    TEN_ASSERT(json_str, "Failed to get JSON string from JSON.");
    std::string res = json_str;

    ten_json_destroy(c_json);
    if (must_free) {
      TEN_FREE(
          json_str);  // NOLINT(cppcoreguidelines-no-malloc, hicpp-no-malloc)
    }
    return res;
  }

  bool from_json(const char *json_str, error_t *err = nullptr) {
    TEN_ASSERT(c_msg_, "Should not happen.");

    if (c_msg_ == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(), TEN_ERRNO_GENERIC,
                      "Invalid TEN message.");
      }
      return false;
    }

    bool result = true;

    ten_json_t *c_json = ten_json_from_string(
        json_str,
        err != nullptr ? err->get_internal_representation() : nullptr);
    if (c_json == nullptr) {
      result = false;
      goto done;  // NOLINT(hicpp-avoid-goto,cppcoreguidelines-avoid-goto)
    }

    result = ten_msg_from_json(
        c_msg_, c_json,
        err != nullptr ? err->get_internal_representation() : nullptr);
    if (!result) {
      TEN_LOGW("Failed to set message content.");
      goto done;  // NOLINT(hicpp-avoid-goto,cppcoreguidelines-avoid-goto)
    }

  done:
    if (c_json != nullptr) {
      ten_json_destroy(c_json);
    }
    return result;
  }

  bool is_property_exist(const char *path, error_t *err = nullptr) {
    TEN_ASSERT(c_msg_, "Should not happen.");
    TEN_ASSERT(path && strlen(path), "path should not be empty.");

    if (c_msg_ == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(),
                      TEN_ERRNO_INVALID_ARGUMENT, "Invalid TEN message.");
      }
      return false;
    }

    return ten_msg_is_property_exist(
        c_msg_, path,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }

  virtual uint8_t get_property_uint8(const char *path, error_t *err) {
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

  virtual uint16_t get_property_uint16(const char *path, error_t *err) {
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

  virtual uint32_t get_property_uint32(const char *path, error_t *err) {
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

  virtual uint64_t get_property_uint64(const char *path, error_t *err) {
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

  virtual int8_t get_property_int8(const char *path, error_t *err) {
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

  virtual int16_t get_property_int16(const char *path, error_t *err) {
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

  virtual int32_t get_property_int32(const char *path, error_t *err) {
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

  virtual int64_t get_property_int64(const char *path, error_t *err) {
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

  virtual float get_property_float32(const char *path, error_t *err) {
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

  virtual double get_property_float64(const char *path, error_t *err) {
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

  virtual std::string get_property_string(const char *path, error_t *err) {
    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return "";
    }
    return ten_value_peek_string(c_value);
  }

  std::string get_property_string(const char *path) {
    return get_property_string(path, nullptr);
  }

  virtual void *get_property_ptr(const char *path, error_t *err) {
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

  virtual bool get_property_bool(const char *path, error_t *err) {
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

  // Pay attention to its copy semantics.
  virtual ten::buf_t get_property_buf(const char *path, error_t *err) {
    auto result = ten::buf_t{};

    ten_value_t *c_value = peek_property_value(path, err);
    if (c_value == nullptr) {
      return result;
    }

    ten_buf_t *c_buf = ten_value_peek_buf(c_value);
    ten_buf_init_with_copying_data(&result.buf, c_buf->data, c_buf->size);

    return result;
  }

  ten::buf_t get_property_buf(const char *path) {
    return get_property_buf(path, nullptr);
  }

  std::string get_property_to_json(const char *path,
                                   error_t *err = nullptr) const {
    TEN_ASSERT(path && strlen(path), "path should not be empty.");

    if (c_msg_ == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(),
                      TEN_ERRNO_INVALID_ARGUMENT, "Invalid TEN message.");
      }
      return "";
    }

    std::string result;

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

  virtual bool set_property(const char *path, int8_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_int8(value), err);
  }

  bool set_property(const char *path, int8_t value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, int16_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_int16(value), err);
  }

  bool set_property(const char *path, int16_t value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, int32_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_int32(value), err);
  }

  bool set_property(const char *path, int32_t value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, int64_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_int64(value), err);
  }

  bool set_property(const char *path, int64_t value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, uint8_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_uint8(value), err);
  }

  bool set_property(const char *path, uint8_t value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, uint16_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_uint16(value), err);
  }

  bool set_property(const char *path, uint16_t value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, uint32_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_uint32(value), err);
  }

  bool set_property(const char *path, uint32_t value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, uint64_t value, error_t *err) {
    return set_property_impl(path, ten_value_create_uint64(value), err);
  }

  bool set_property(const char *path, uint64_t value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, float value, error_t *err) {
    return set_property_impl(path, ten_value_create_float32(value), err);
  }

  bool set_property(const char *path, float value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, double value, error_t *err) {
    return set_property_impl(path, ten_value_create_float64(value), err);
  }

  bool set_property(const char *path, double value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, bool value, error_t *err) {
    return set_property_impl(path, ten_value_create_bool(value), err);
  }

  bool set_property(const char *path, bool value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, void *value, error_t *err) {
    if (value == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(),
                      TEN_ERRNO_INVALID_ARGUMENT, "Invalid argment.");
      }
      return false;
    }
    return set_property_impl(
        path, ten_value_create_ptr(value, nullptr, nullptr, nullptr), err);
  }

  bool set_property(const char *path, void *value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, const char *value, error_t *err) {
    if (value == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(),
                      TEN_ERRNO_INVALID_ARGUMENT, "Invalid argment.");
      }
      return false;
    }
    return set_property_impl(path, ten_value_create_string(value), err);
  }

  bool set_property(const char *path, const char *value) {
    return set_property(path, value, nullptr);
  }

  virtual bool set_property(const char *path, const std::string &value,
                            error_t *err) {
    return set_property_impl(path, ten_value_create_string(value.c_str()), err);
  }

  bool set_property(const char *path, const std::string &value) {
    return set_property(path, value, nullptr);
  }

  // Pay attention to its copy semantics.
  virtual bool set_property(const char *path, const ten::buf_t &value,
                            error_t *err) {
    if (value.data() == nullptr) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(),
                      TEN_ERRNO_INVALID_ARGUMENT, "Invalid argment.");
      }
      return false;
    }
    ten_buf_t buf;
    ten_buf_init_with_copying_data(&buf, value.data(), value.size());
    return set_property_impl(path, ten_value_create_buf_with_move(buf), err);
  }

  bool set_property(const char *path, const ten::buf_t &value) {
    return set_property(path, value, nullptr);
  }

  bool set_property_from_json(const char *path, const char *json,
                              error_t *err = nullptr) {
    TEN_ASSERT(c_msg_, "Should not happen.");

    ten_json_t *c_json = ten_json_from_string(
        json, err != nullptr ? err->get_internal_representation() : nullptr);
    if (c_json == nullptr) {
      return false;
    }

    ten_value_t *value = ten_value_from_json(c_json);
    ten_json_destroy(c_json);

    return set_property_impl(path, value, err);
  }

  // Internal use only.
  ten_shared_ptr_t *get_underlying_msg() const { return c_msg_; }

 protected:
  friend class ten_env_t;
  friend class cmd_result_t;

  // @{
  // Used by the constructor of the real command class to create a base command
  // first.
  msg_t() = default;
  explicit msg_t(ten_shared_ptr_t *msg) : c_msg_(msg) {}
  // @}

  /**
   * @note Note the move semantics of @a value. The @a value should not be
   * used after calling this function.
   */
  bool set_property_impl(const char *path, ten_value_t *value,
                         error_t *err = nullptr) {
    TEN_ASSERT(c_msg_, "Should not happen.");

    bool rc = ten_msg_set_property(
        c_msg_, path, value,
        err != nullptr ? err->get_internal_representation() : nullptr);

    if (!rc) {
      ten_value_destroy(value);
    }
    return rc;
  }

  void relinquish_underlying_msg() {
    if (c_msg_ != nullptr) {
      ten_shared_ptr_destroy(c_msg_);
      c_msg_ = nullptr;
    }
  }

  ten_shared_ptr_t *c_msg_ = nullptr;  // NOLINT

 private:
  ten_value_t *peek_property_value(const char *path, error_t *err) const {
    TEN_ASSERT(c_msg_, "Should not happen.");

    return ten_msg_peek_property(
        c_msg_, path,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }
};

// NOLINTEND(cert-dcl50-cpp)

}  // namespace ten
