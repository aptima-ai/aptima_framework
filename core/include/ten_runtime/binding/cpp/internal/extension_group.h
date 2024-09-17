//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include <cstddef>

#include "ten_utils/macro/check.h"
#include "ten_runtime/binding/common.h"
#include "ten_runtime/binding/cpp/internal/ten_env.h"
#include "ten_runtime/extension_group/extension_group.h"
#include "ten_utils/container/list.h"

// NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
using ten_extension_group_t = struct ten_extension_group_t;
using ten_env_t = struct ten_env_t;

namespace ten {

class ten_env_t;
class app_t;

class extension_group_t {
 public:
  virtual ~extension_group_t() {
    TEN_ASSERT(c_extension_group_, "Should not happen.");
    ten_extension_group_destroy(c_extension_group_);

    TEN_ASSERT(cpp_ten_env, "Should not happen.");
    delete cpp_ten_env;
  }

  // @{
  extension_group_t(const extension_group_t &) = delete;
  extension_group_t(extension_group_t &&) = delete;
  extension_group_t &operator=(const extension_group_t &) = delete;
  extension_group_t &operator=(extension_group_t &&) = delete;
  // @}

  // @{
  // Internal use only.
  ::ten_extension_group_t *get_c_extension_group() const {
    return c_extension_group_;
  }
  // @}

 protected:
  explicit extension_group_t(const std::string &name)
      : c_extension_group_(ten_extension_group_create(
            name.c_str(), proxy_on_init, proxy_on_deinit,
            proxy_on_create_extensions, proxy_on_destroy_extensions)),
        cpp_ten_env(new ten_env_t(
            ten_extension_group_get_ten_env(c_extension_group_))) {
    ten_binding_handle_set_me_in_target_lang(
        reinterpret_cast<ten_binding_handle_t *>(c_extension_group_), this);

    TEN_ASSERT(cpp_ten_env, "Should not happen.");
  }

  virtual void on_init(ten_env_t &ten_env) { ten_env.on_init_done(); }

  virtual void on_deinit(ten_env_t &ten_env) { ten_env.on_deinit_done(); }

  virtual void on_create_extensions(ten_env_t &ten_env) {
    std::vector<extension_t *> const extensions;
    ten_env.on_create_extensions_done(extensions);
  }

  virtual void on_destroy_extensions(
      TEN_UNUSED ten_env_t &ten_env,
      const std::vector<extension_t *> & /* extensions */) {
    TEN_ASSERT(0 && "should be overriden by the child class.",
               "Should not happen.");
  }

 private:
  friend class ten_env_t;
  friend class app_t;

  static void proxy_on_init(ten_extension_group_t *extension_group,
                            ::ten_env_t *ten_env) {
    TEN_ASSERT(extension_group &&
                   ten_extension_group_check_integrity(extension_group, true),
               "Invalid argument.");
    TEN_ASSERT(ten_extension_group_get_ten_env(extension_group) &&
                   ten_env_check_integrity(
                       ten_extension_group_get_ten_env(extension_group), true),
               "Should not happen.");

    auto *cpp_extension_group = static_cast<extension_group_t *>(
        ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(extension_group)));

    auto *cpp_ten_env =
        static_cast<ten_env_t *>(ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(ten_env)));

    cpp_extension_group->on_init(*cpp_ten_env);
  }

  static void proxy_on_deinit(ten_extension_group_t *extension_group,
                              ::ten_env_t *ten_env) {
    TEN_ASSERT(extension_group &&
                   ten_extension_group_check_integrity(extension_group, true),
               "Invalid argument.");
    TEN_ASSERT(ten_extension_group_get_ten_env(extension_group) &&
                   ten_env_check_integrity(
                       ten_extension_group_get_ten_env(extension_group), true),
               "Should not happen.");

    auto *cpp_extension_group = static_cast<extension_group_t *>(
        ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(extension_group)));

    auto *cpp_ten_env =
        static_cast<ten_env_t *>(ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(ten_env)));

    cpp_extension_group->on_deinit(*cpp_ten_env);
  }

  static void proxy_on_create_extensions(ten_extension_group_t *extension_group,
                                         ::ten_env_t *ten_env) {
    TEN_ASSERT(extension_group &&
                   ten_extension_group_check_integrity(extension_group, true),
               "Invalid argument.");
    TEN_ASSERT(ten_extension_group_get_ten_env(extension_group) &&
                   ten_env_check_integrity(
                       ten_extension_group_get_ten_env(extension_group), true),
               "Should not happen.");

    auto *cpp_extension_group = static_cast<extension_group_t *>(
        ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(extension_group)));
    TEN_ASSERT(cpp_extension_group, "Should not happen.");

    auto *cpp_ten_env =
        static_cast<ten_env_t *>(ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(ten_env)));

    cpp_extension_group->on_create_extensions(*cpp_ten_env);
  }

  static void proxy_on_destroy_extensions(
      ten_extension_group_t *extension_group, ::ten_env_t *ten_env,
      ::ten_list_t extensions) {
    TEN_ASSERT(extension_group &&
                   ten_extension_group_check_integrity(extension_group, true),
               "Invalid argument.");
    TEN_ASSERT(ten_extension_group_get_ten_env(extension_group) &&
                   ten_env_check_integrity(
                       ten_extension_group_get_ten_env(extension_group), true),
               "Should not happen.");

    auto *cpp_extension_group = static_cast<extension_group_t *>(
        ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(extension_group)));
    TEN_ASSERT(cpp_extension_group, "Should not happen.");

    std::vector<extension_t *> extensions_vector;
    ten_list_foreach (&extensions, iter) {
      auto *c_extension =
          static_cast<::ten_extension_t *>(ten_ptr_listnode_get(iter.node));
      TEN_ASSERT(
          c_extension && ten_extension_check_integrity(c_extension, true),
          "Should not happen.");

      extensions_vector.push_back(
          static_cast<extension_t *>(ten_binding_handle_get_me_in_target_lang(
              reinterpret_cast<ten_binding_handle_t *>(c_extension))));
    }

    auto *cpp_ten_env =
        static_cast<ten_env_t *>(ten_binding_handle_get_me_in_target_lang(
            reinterpret_cast<ten_binding_handle_t *>(ten_env)));

    // The extension group should be responsible for deleting the C++
    // ten_extension_t object.
    cpp_extension_group->on_destroy_extensions(*cpp_ten_env, extensions_vector);
  }

  ::ten_extension_group_t *c_extension_group_;
  ten_env_t *cpp_ten_env;
};

}  // namespace ten
