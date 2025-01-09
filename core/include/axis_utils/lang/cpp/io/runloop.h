//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <memory>
#include <string>

#include "aptima_utils/io/runloop.h"

namespace ten {

class Runloop;
using TenRunloop = std::unique_ptr<Runloop>;

class Runloop {
 public:
  static TenRunloop Create(const std::string &impl = "") {
    auto loop = aptima_runloop_create(impl.empty() ? nullptr : impl.c_str());
    if (!loop) {
      return nullptr;
    }

    return std::unique_ptr<Runloop>(new (std::nothrow) Runloop(loop));
  }

  Runloop() = delete;

  Runloop(const Runloop &rhs) = delete;
  Runloop &operator=(const Runloop &rhs) = delete;

  Runloop(Runloop &&rhs) = delete;

  Runloop &operator=(Runloop &&rhs) = delete;

  ~Runloop() { aptima_runloop_destroy(loop_); }

 public:
  ::aptima_runloop_t *get_c_loop() const { return loop_; }

  void Run() { aptima_runloop_run(loop_); }

  void Stop() { aptima_runloop_stop(loop_); }

  bool Alive() const { return aptima_runloop_alive(loop_) == 1; }

 private:
  explicit Runloop(::aptima_runloop_t *loop) : loop_(loop) {}

 private:
  ::aptima_runloop_t *loop_;
};

}  // namespace ten
