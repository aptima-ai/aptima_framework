//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <cstddef>
#include <cstdint>
#include <memory>

#include "ten_runtime/binding/cpp/internal/msg/msg.h"
#include "ten_runtime/msg/video_frame/video_frame.h"
#include "ten_utils/lib/smart_ptr.h"

namespace ten {

class extension_t;

class video_frame_t : public msg_t {
 private:
  friend extension_t;

  // Passkey Idiom.
  struct ctor_passkey_t {
   private:
    friend video_frame_t;

    explicit ctor_passkey_t() = default;
  };

 public:
  static std::unique_ptr<video_frame_t> create(const char *video_frame_name,
                                               error_t *err = nullptr) {
    if (video_frame_name == nullptr || strlen(video_frame_name) == 0) {
      if (err != nullptr && err->get_internal_representation() != nullptr) {
        ten_error_set(err->get_internal_representation(),
                      TEN_ERRNO_INVALID_ARGUMENT,
                      "Video frame name cannot be empty.");
      }
      return nullptr;
    }

    auto *c_frame = ten_video_frame_create();
    ten_msg_set_name(
        c_frame, video_frame_name,
        err != nullptr ? err->get_internal_representation() : nullptr);

    return std::make_unique<video_frame_t>(c_frame, ctor_passkey_t());
  }

  static std::unique_ptr<video_frame_t> create_from_json(const char *json_str,
                                                         error_t *err = nullptr)
      __attribute__((warning("This method may access the '_ten' field. Use "
                             "caution if '_ten' is provided."))) {
    ten_shared_ptr_t *c_video_frame = ten_video_frame_create_from_json_string(
        json_str,
        err != nullptr ? err->get_internal_representation() : nullptr);

    return std::make_unique<video_frame_t>(c_video_frame, ctor_passkey_t());
  }

  explicit video_frame_t(ten_shared_ptr_t *video_frame,
                         ctor_passkey_t /*unused*/)
      : msg_t(video_frame) {}

  ~video_frame_t() override = default;

  int32_t get_width(error_t *err = nullptr) const {
    return ten_video_frame_get_width(c_msg_);
  }
  bool set_width(int32_t width, error_t *err = nullptr) {
    return ten_video_frame_set_width(c_msg_, width);
  }

  int32_t get_height(error_t *err = nullptr) const {
    return ten_video_frame_get_height(c_msg_);
  }
  bool set_height(int32_t height, error_t *err = nullptr) const {
    return ten_video_frame_set_height(c_msg_, height);
  }

  int64_t get_timestamp(error_t *err = nullptr) const {
    return ten_video_frame_get_timestamp(c_msg_);
  }
  bool set_timestamp(int64_t timestamp, error_t *err = nullptr) const {
    return ten_video_frame_set_timestamp(c_msg_, timestamp);
  }

  TEN_PIXEL_FMT get_pixel_fmt(error_t *err = nullptr) const {
    return ten_video_frame_get_pixel_fmt(c_msg_);
  }
  bool set_pixel_fmt(TEN_PIXEL_FMT pixel_fmt, error_t *err = nullptr) const {
    return ten_video_frame_set_pixel_fmt(c_msg_, pixel_fmt);
  }

  bool is_eof(error_t *err = nullptr) const {
    return ten_video_frame_is_eof(c_msg_);
  }
  bool set_is_eof(bool is_eof, error_t *err = nullptr) {
    return ten_video_frame_set_is_eof(c_msg_, is_eof);
  }

  bool alloc_buf(size_t size, error_t *err = nullptr) {
    return ten_video_frame_alloc_data(c_msg_, size) != nullptr;
  }

  buf_t lock_buf(error_t *err = nullptr) const {
    if (!ten_msg_add_locked_res_buf(
            c_msg_, ten_video_frame_peek_data(c_msg_)->data,
            err != nullptr ? err->get_internal_representation() : nullptr)) {
      return buf_t{};
    }

    buf_t result{ten_video_frame_peek_data(c_msg_)->data,
                 ten_video_frame_peek_data(c_msg_)->size};

    return result;
  }

  bool unlock_buf(buf_t &buf, error_t *err = nullptr) {
    const uint8_t *data = buf.data();
    if (!ten_msg_remove_locked_res_buf(
            c_msg_, data,
            err != nullptr ? err->get_internal_representation() : nullptr)) {
      return false;
    }

    // Since the `buf` has already been given back, clearing the contents of the
    // `buf` itself not only notifies developers that this `buf` can no longer
    // be used, but also prevents it from being used incorrectly again.
    ten_buf_init_with_owned_data(&buf.buf, 0);

    return true;
  }

  // @{
  video_frame_t(const video_frame_t &other) = delete;
  video_frame_t(video_frame_t &&other) noexcept = delete;
  video_frame_t &operator=(const video_frame_t &other) = delete;
  video_frame_t &operator=(video_frame_t &&other) noexcept = delete;
  // @}

  // @{
  // Internal use only. This function is called in 'extension_t' to create C++
  // message from C message.
  explicit video_frame_t(ten_shared_ptr_t *frame) : msg_t(frame) {}
  // @}
};

}  // namespace ten
