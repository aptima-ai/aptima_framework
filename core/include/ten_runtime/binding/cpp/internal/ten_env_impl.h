//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <vector>

#include "ten_runtime/binding/cpp/internal/addon.h"
#include "ten_runtime/binding/cpp/internal/extension.h"
#include "ten_runtime/binding/cpp/internal/extension_group.h"
#include "ten_runtime/binding/cpp/internal/ten_env.h"
#include "ten_utils/lang/cpp/lib/error.h"

namespace ten {

class ten_env_t;
class extension_t;

inline bool ten_env_t::on_create_instance_done(void *instance, void *context,
                                               error_t *err) {
  void *c_instance = nullptr;

  auto *cpp_context = reinterpret_cast<addon_context_t *>(context);
  TEN_ASSERT(cpp_context, "Invalid argument.");

  switch (cpp_context->task) {
    case ADDON_TASK_CREATE_EXTENSION:
      c_instance = static_cast<extension_t *>(instance)->get_c_extension();
      break;
    case ADDON_TASK_CREATE_EXTENSION_GROUP:
      c_instance =
          static_cast<extension_group_t *>(instance)->get_c_extension_group();
      break;
    default:
      TEN_ASSERT(0, "Should not happen.");
      break;
  }

  TEN_ASSERT(c_instance, "Should not happen.");

  bool rc = ten_env_on_create_instance_done(
      c_ten_env, c_instance, cpp_context->c_context,
      err != nullptr ? err->get_internal_representation() : nullptr);

  delete cpp_context;

  return rc;
}

inline bool ten_env_t::on_create_extensions_done(
    const std::vector<extension_t *> &extensions, error_t *err) {
  ten_list_t c_extensions = TEN_LIST_INIT_VAL;

  for (const auto &extension : extensions) {
    ten_list_push_ptr_back(&c_extensions, extension->get_c_extension(),
                           nullptr);
  }

  return ten_env_on_create_extensions_done(
      c_ten_env, &c_extensions,
      err != nullptr ? err->get_internal_representation() : nullptr);
}

inline bool ten_env_t::addon_destroy_extension_async(
    ten::extension_t *extension, addon_destroy_extension_async_cb_t &&cb,
    error_t *err) {
  if (cb == nullptr) {
    return ten_addon_destroy_extension_async(
        c_ten_env, extension->get_c_extension(), nullptr, nullptr,
        err != nullptr ? err->get_internal_representation() : nullptr);
  } else {
    auto *cb_ptr = new addon_destroy_extension_async_cb_t(std::move(cb));

    return ten_addon_destroy_extension_async(
        c_ten_env, extension->get_c_extension(),
        proxy_addon_destroy_extension_async_cb, cb_ptr,
        err != nullptr ? err->get_internal_representation() : nullptr);
  }
}

}  // namespace ten
