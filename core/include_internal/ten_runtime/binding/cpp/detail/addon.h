//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <cassert>
#include <cstddef>

#include "ten_runtime/binding/cpp/detail/addon.h"
#include "ten_runtime/binding/cpp/detail/ten_env.h"

namespace ten {

class addon_loader_addon_t : public addon_t {
 protected:
  void on_load(ten_env_t &ten_env, const char *name, void *context) override {
    (void)ten_env;
    (void)name;
    (void)context;

    // If a subclass requires the functionality of this function, it needs to
    // override this function.
    TEN_ASSERT(0, "Should not happen.");
  }
};

}  // namespace ten
