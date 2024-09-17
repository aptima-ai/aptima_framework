//
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file for more information.
//
#include <cstdlib>
#include <string>
#include <vector>

#include "ten_runtime/binding/cpp/ten.h"

namespace ten {
namespace ffmpeg_extension {

class ffmpeg_extension_group_t : public extension_group_t {
 public:
  explicit ffmpeg_extension_group_t(const char *name)
      : extension_group_t(name) {}

  ~ffmpeg_extension_group_t() override = default;

  ffmpeg_extension_group_t(const ffmpeg_extension_group_t &other) = delete;
  ffmpeg_extension_group_t(ffmpeg_extension_group_t &&other) = delete;

  ffmpeg_extension_group_t &operator=(const ffmpeg_extension_group_t &other) =
      delete;
  ffmpeg_extension_group_t &operator=(ffmpeg_extension_group_t &&other) =
      delete;

  void on_create_extensions(ten::ten_env_t &ten_env) override {
    auto extensions = std::make_shared<std::vector<ten::extension_t *>>();

    // Create 'ffmpeg muxer' extension.
    if (!ten_env.addon_create_extension_async(
            "ffmpeg_muxer", "ffmpeg_muxer",
            [extensions](ten::ten_env_t &ten_env, ten::extension_t &extension) {
              extensions->push_back(&extension);

              if (extensions->size() == 2) {
                ten_env.on_create_extensions_done(*extensions);
              }
            })) {
      TEN_LOGE("Failed to find the addon for extension ffmpeg_muxer");
#if defined(_DEBUG)
      TEN_ASSERT(0, "Should not happen.");
#endif
    }

    // Create 'ffmpeg demuxer' extension.
    if (!ten_env.addon_create_extension_async(
            "ffmpeg_demuxer", "ffmpeg_demuxer",
            [extensions](ten::ten_env_t &ten_env, ten::extension_t &extension) {
              extensions->push_back(&extension);

              if (extensions->size() == 2) {
                ten_env.on_create_extensions_done(*extensions);
              }
            })) {
      TEN_LOGE("Failed to find the addon for extension ffmpeg_demuxer");
#if defined(_DEBUG)
      TEN_ASSERT(0, "Should not happen.");
#endif
    }
  }

  void on_destroy_extensions(
      ten_env_t &ten_env,
      const std::vector<extension_t *> &extensions) override {
    for (auto *extension : extensions) {
      delete extension;
    }
    ten_env.on_destroy_extensions_done();
  }

 private:
  void on_addon_create_extension_done(ten::ten_env_t &ten_env,
                                      ten::extension_t &extension,
                                      void *user_data) {}
};

TEN_CPP_REGISTER_ADDON_AS_EXTENSION_GROUP(ffmpeg_extension_group,
                                          ffmpeg_extension_group_t);

}  // namespace ffmpeg_extension
}  // namespace ten
