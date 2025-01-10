//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "buf.h"
#include "aptima_runtime/common/errno.h"
#include "aptima_utils/lib/alloc.h"
#include "aptima_utils/lib/buf.h"
#include "aptima_utils/lib/error.h"
#include "aptima_utils/lib/json.h"
#include "aptima_utils/lib/string.h"
#include "aptima_utils/macro/check.h"
#include "aptima_utils/value/value.h"
#include "aptima_utils/value/value_get.h"
#include "aptima_utils/value/value_is.h"

using aptima_value_t = struct ::aptima_value_t;

namespace aptima {

class aptima_env_t;
class msg_t;
class cmd_result_t;
class value_kv_t;

class value_t {
 public:
  value_t() = default;

  value_t(const value_t &other) { *this = other; }

  value_t(value_t &&other) noexcept { *this = std::move(other); }

  ~value_t() {
    if (c_value_ != nullptr && own_) {
      aptima_value_destroy(c_value_);
    }
  }

 private:
  explicit value_t(bool value) : c_value_(aptima_value_create_bool(value)) {}

  explicit value_t(int8_t value) : c_value_(aptima_value_create_int8(value)) {}

  explicit value_t(int16_t value) : c_value_(aptima_value_create_int16(value)) {}

  explicit value_t(int32_t value) : c_value_(aptima_value_create_int32(value)) {}

  explicit value_t(int64_t value) : c_value_(aptima_value_create_int64(value)) {}

  explicit value_t(uint8_t value) : c_value_(aptima_value_create_uint8(value)) {}

  explicit value_t(uint16_t value) : c_value_(aptima_value_create_uint16(value)) {}

  explicit value_t(uint32_t value) : c_value_(aptima_value_create_uint32(value)) {}

  explicit value_t(uint64_t value) : c_value_(aptima_value_create_uint64(value)) {}

  explicit value_t(float value) : c_value_(aptima_value_create_float32(value)) {}

  explicit value_t(double value) : c_value_(aptima_value_create_float64(value)) {}

  // @{
  // Create a APTIMA value of 'ptr' type for all types of C++ pointers.
  template <typename V>
  explicit value_t(V *value) {
    if (value == nullptr) {
      c_value_ = aptima_value_create_invalid();
    } else {
      c_value_ = aptima_value_create_ptr(value, nullptr, nullptr, nullptr);
    }
  }
  // @}

  template <typename T>
  struct always_false : public std::false_type {};

  template <typename T>
  struct is_vector : public std::false_type {};

  template <typename T, typename A>
  struct is_vector<std::vector<T, A>> : public std::true_type {};

  template <typename T>
  struct is_map : public std::false_type {};

  template <typename T, typename A>
  struct is_map<std::map<T, A>> : public std::true_type {};

  /**
   * @brief This is the fallback constructor to handle all other types of C++
   * objects.
   */
  template <
      typename V,
      // Have specific constructors to handle aptima::value_t.
      typename std::enable_if<
          !std::is_same<typename std::remove_reference<V>::type,
                        value_t>::value,
          void>::type * = nullptr,
      // Have specific constructors to handle std::vector.
      typename std::enable_if<
          !is_vector<typename std::remove_reference<V>::type>::value,
          void>::type * = nullptr,
      // Have specific constructors to handle std::map.
      typename std::enable_if<
          !is_map<typename std::remove_reference<V>::type>::value, void>::type
          * = nullptr,
      // Have specific constructors to handle normal pointers.
      typename std::enable_if<
          !std::is_pointer<typename std::remove_reference<V>::type>::value,
          void>::type * = nullptr,
      // Have specific constructors to handle std::string.
      typename std::enable_if<
          !std::is_same<typename std::remove_reference<V>::type,
                        std::string>::value,
          void>::type * = nullptr>
  value_t(V &&value) {  // NOLINT
    static_assert(always_false<V>::value,
                  "This type is not supported by APTIMA C++ Binding.");
  }

  // @{
  // Pay attention to its copy semantics.
  explicit value_t(const std::string &value)
      : c_value_(aptima_value_create_string(value.c_str())) {}

  explicit value_t(const char *value) {
    if (value == nullptr) {
      c_value_ = aptima_value_create_invalid();
    } else {
      c_value_ = aptima_value_create_string(value);
    }
  }
  // @}

  // Pay attention to its copy semantics.
  explicit value_t(const buf_t &value) {
    if (value.size() == 0) {
      c_value_ = aptima_value_create_invalid();
      return;
    }

    aptima_buf_t buf;
    aptima_buf_init_with_owned_data(&buf, value.buf.size);

    memcpy(buf.data, value.buf.data, value.buf.size);

    c_value_ = aptima_value_create_buf_with_move(buf);
  }

  // Create a APTIMA value of 'object' type.
  template <typename V>
  explicit value_t(const std::map<std::string, V> &map) {
    aptima_list_t m = aptima_LIST_INIT_VAL;

    for (const auto &pair : map) {
      aptima_value_kv_t *aptima_pair = aptima_value_kv_create_empty(pair.first.c_str());
      aptima_ASSERT(aptima_pair, "Invalid argument.");

      aptima_pair->value = create_c_value_from_cpp_concept(pair.second);
      aptima_list_push_ptr_back(&m, aptima_pair,
                             reinterpret_cast<aptima_ptr_listnode_destroy_func_t>(
                                 aptima_value_kv_destroy));
    }

    c_value_ = aptima_value_create_object_with_move(&m);
    aptima_list_clear(&m);
  }

  // Create a APTIMA value of 'array' type.
  template <typename V>
  explicit value_t(const std::vector<V> &list) {
    aptima_list_t m = aptima_LIST_INIT_VAL;

    for (const auto &v : list) {
      aptima_list_push_ptr_back(
          &m, create_c_value_from_cpp_concept(v),
          reinterpret_cast<aptima_ptr_listnode_destroy_func_t>(aptima_value_destroy));
    }

    c_value_ = aptima_value_create_array_with_move(&m);
    aptima_list_clear(&m);
  }

  // Create a APTIMA value of 'array' type.
  template <typename V>
  explicit value_t(const std::unordered_set<V> &list) {
    aptima_list_t m = aptima_LIST_INIT_VAL;

    for (const auto &v : list) {
      aptima_list_push_ptr_back(
          &m, create_c_value_from_cpp_concept(v),
          reinterpret_cast<aptima_ptr_listnode_destroy_func_t>(aptima_value_destroy));
    }

    c_value_ = aptima_value_create_array_with_move(&m);
    aptima_list_clear(&m);
  }

  // Copy semantics.
  value_t &operator=(const value_t &other) {
    if (this == &other) {
      return *this;
    }

    if (c_value_ != nullptr && own_) {
      // Destroy the old value.
      aptima_value_destroy(c_value_);
    }

    c_value_ = nullptr;
    own_ = false;

    if (other.c_value_ != nullptr) {
      if (other.own_) {
        c_value_ = aptima_value_clone(other.c_value_);
      } else {
        c_value_ = other.c_value_;
      }

      own_ = other.own_;
    }

    return *this;
  }

  // Move semantics.
  value_t &operator=(value_t &&other) noexcept {
    if (c_value_ != nullptr && own_) {
      // Destroy the old value.
      aptima_value_destroy(c_value_);
    }

    c_value_ = nullptr;
    own_ = true;

    if (other.c_value_ != nullptr) {
      c_value_ = other.c_value_;
      other.c_value_ = nullptr;

      own_ = other.own_;
    }

    return *this;
  }

  bool is_valid() const {
    // The 'c_value_' is always not NULL.
    aptima_ASSERT(c_value_, "Should not happen.");
    return aptima_value_is_valid(c_value_);
  }

  bool from_json(aptima_json_t *c_json) {
    if (c_json == nullptr) {
      return false;
    }

    if (c_value_ != nullptr && own_) {
      aptima_value_destroy(c_value_);
    }

    c_value_ = aptima_value_from_json(c_json);
    return c_value_ != nullptr;
  }

  aptima_TYPE get_type() const {
    if (c_value_ == nullptr) {
      return aptima_TYPE_INVALID;
    }
    return aptima_value_get_type(c_value_);
  }

  template <typename T>
  T get_real_value(aptima_error_t *err = nullptr) {
    auto *tmp = err;
    if (tmp == nullptr) {
      tmp = aptima_error_create();
    }

    auto t = get_real_value_impl<T>(tmp);
    if (err == nullptr) {
      aptima_error_destroy(tmp);
    }

    return t;
  }

  bool get_bool(aptima_error_t *err) const {
    return aptima_value_get_bool(c_value_, err);
  }

  int8_t get_int8(aptima_error_t *err) const {
    return aptima_value_get_int8(c_value_, err);
  }

  int16_t get_int16(aptima_error_t *err) const {
    return aptima_value_get_int16(c_value_, err);
  }

  int32_t get_int32(aptima_error_t *err) const {
    return aptima_value_get_int32(c_value_, err);
  }

  int64_t get_int64(aptima_error_t *err) const {
    return aptima_value_get_int64(c_value_, err);
  }

  uint8_t get_uint8(aptima_error_t *err) const {
    return aptima_value_get_uint8(c_value_, err);
  }

  uint16_t get_uint16(aptima_error_t *err) const {
    return aptima_value_get_uint16(c_value_, err);
  }

  uint32_t get_uint32(aptima_error_t *err) const {
    return aptima_value_get_uint32(c_value_, err);
  }

  uint64_t get_uint64(aptima_error_t *err) const {
    return aptima_value_get_uint64(c_value_, err);
  }

  float get_float32(aptima_error_t *err) const {
    return aptima_value_get_float32(c_value_, err);
  }

  double get_float64(aptima_error_t *err) const {
    return aptima_value_get_float64(c_value_, err);
  }

  void *get_ptr(aptima_error_t *err) const {
    return aptima_value_get_ptr(c_value_, err);
  }

  std::vector<value_t> get_array(const std::vector<value_t> &default_value) {
    if (c_value_ == nullptr) {
      return default_value;
    }

    if (!aptima_value_is_array(c_value_)) {
      return default_value;
    }

    std::vector<value_t> result;
    auto size = aptima_value_array_size(c_value_);
    for (size_t i = 0; i < size; ++i) {
      auto *v = aptima_value_array_peek(c_value_, i, nullptr);
      aptima_ASSERT(v, "Invalid argument.");

      result.push_back(value_t(aptima_value_clone(v)));
    }

    return result;
  }

  std::map<std::string, value_t> get_object(
      const std::map<std::string, value_t> &default_value) {
    if (c_value_ == nullptr) {
      return default_value;
    }

    if (!aptima_value_is_object(c_value_)) {
      return default_value;
    }

    std::map<std::string, value_t> result;
    aptima_value_object_foreach(c_value_, iter) {
      auto *kv =
          reinterpret_cast<aptima_value_kv_t *>(aptima_ptr_listnode_get(iter.node));
      aptima_ASSERT(kv, "Invalid argument.");

      result[aptima_string_get_raw_str(&kv->key)] =
          value_t(aptima_value_clone(kv->value));
    };

    return result;
  }

  std::string get_string(aptima_error_t *err) const {
    aptima_ASSERT(c_value_, "Should not happen.");

    (void)err;

    // TODO(Liu): pass `err` to the underlying function.
    const char *result = aptima_value_peek_raw_str(c_value_, err);
    if (result != nullptr) {
      return result;
    } else {
      // Property is not found.
      aptima_error_set(err, aptima_ERRNO_GENERIC, "Not found.");
      return {};
    }
  }

  // Pay attention to its copy semantics.
  buf_t get_buf(aptima_error_t *err) const {
    aptima_ASSERT(c_value_, "Should not happen.");

    (void)err;

    void *orig_data = nullptr;
    size_t orig_size = 0;

    // TODO(Liu): pass `err` to the underlying function.
    aptima_buf_t *result = aptima_value_peek_buf(c_value_);
    if (result != nullptr) {
      orig_size = result->content_size;
      orig_data = result->data;
    }

    buf_t buf{orig_size};
    memcpy(buf.data(), orig_data, orig_size);

    return buf;
  }

  int to_json(std::string &result) const {
    aptima_json_t *c_json = aptima_value_to_json(c_value_);
    if (c_json == nullptr) {
      return -1;
    }

    bool must_free = false;
    const char *json_str = aptima_json_to_string(c_json, nullptr, &must_free);
    aptima_ASSERT(json_str, "Failed to convert a JSON to a string");

    result = json_str;

    aptima_json_destroy(c_json);
    if (must_free) {
      aptima_FREE(json_str);
    }

    return 0;
  }

  friend class aptima_env_t;
  friend class msg_t;
  friend class cmd_result_t;
  friend class value_kv_t;

  template <typename T, typename std::enable_if<
                            !std::is_pointer<T>::value>::type * = nullptr>
  T get_real_value_impl(aptima_error_t *err) {
    (void)err;
    static_assert(always_false<T>::value,
                  "This type is not supported by APTIMA C++ Binding.");
  }

  template <typename T, typename std::enable_if<std::is_pointer<T>::value>::type
                            * = nullptr>
  T get_real_value_impl(aptima_error_t *err) {
    return static_cast<T>(get_ptr(err));
  }

  template <typename T>
  ::aptima_value_t *create_c_value_from_cpp_concept(const T &v) {
    value_t tmp(v);
    ::aptima_value_t *ret = tmp.c_value_;
    tmp.c_value_ = nullptr;
    return ret;
  }

  // @{
  // These functions are used internally in APTIMA runtime.

  // One C++ value_t instance is by default corresponding one-to-one with one C
  // aptima_value_t instance, meaning the C++ value_t instance will by default own
  // the C aptima_value_t instance, unless for performance considerations,
  // disowning is used in the internal implementation of the APTIMA runtime. In
  // interactions with extensions, the state is always owned, which means, when
  // one C++ value_t instance is destructed, the associated C aptima_value_t
  // instance will be destroyed along with it.
  explicit value_t(::aptima_value_t *c_value, bool own = true)
      : own_(own), c_value_(c_value) {}

  aptima_value_t *get_c_value() const { return c_value_; }

  // @}

  bool own_ = true;
  ::aptima_value_t *c_value_ = nullptr;
};

// @{
// Special specialization of 'get_real_value_impl'.
template <>
inline bool value_t::get_real_value_impl<bool>(aptima_error_t *err) {
  return get_bool(err);
}

template <>
inline int8_t value_t::get_real_value_impl<int8_t>(aptima_error_t *err) {
  return get_int8(err);
}

template <>
inline int16_t value_t::get_real_value_impl<int16_t>(aptima_error_t *err) {
  return get_int16(err);
}

template <>
inline int32_t value_t::get_real_value_impl<int32_t>(aptima_error_t *err) {
  return get_int32(err);
}

template <>
inline int64_t value_t::get_real_value_impl<int64_t>(aptima_error_t *err) {
  return get_int64(err);
}

template <>
inline uint8_t value_t::get_real_value_impl<uint8_t>(aptima_error_t *err) {
  return get_uint8(err);
}

template <>
inline uint16_t value_t::get_real_value_impl<uint16_t>(aptima_error_t *err) {
  return get_uint16(err);
}

template <>
inline uint32_t value_t::get_real_value_impl<uint32_t>(aptima_error_t *err) {
  return get_uint32(err);
}

template <>
inline uint64_t value_t::get_real_value_impl<uint64_t>(aptima_error_t *err) {
  return get_uint64(err);
}

template <>
inline float value_t::get_real_value_impl<float>(aptima_error_t *err) {
  return get_float32(err);
}

template <>
inline double value_t::get_real_value_impl<double>(aptima_error_t *err) {
  return get_float64(err);
}

template <>
inline std::string value_t::get_real_value_impl<std::string>(aptima_error_t *err) {
  return get_string(err);
}

// Pay attention to its copy semantics.
template <>
inline buf_t value_t::get_real_value_impl<buf_t>(aptima_error_t *err) {
  return get_buf(err);
}
// @}

}  // namespace aptima
