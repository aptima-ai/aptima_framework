//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

// This header file should be the only header file where outside world should
// include in the C++ programming language.

// clang-format off
#include "ten_runtime/binding/cpp/internal/common.h"   // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/ten_env.h"      // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/ten_env_mock.h" // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/ten_env_proxy.h" // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/msg/msg.h"  // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/msg/cmd/cmd.h" // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/msg/cmd/start_graph.h" // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/msg/cmd/stop_graph.h" // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/msg/cmd/close_app.h" // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/msg/cmd_result.h" // IWYU pragma: export
#include "include_internal/ten_runtime/binding/cpp/internal/msg/cmd/timeout.h" // IWYU pragma: export
#include "include_internal/ten_runtime/binding/cpp/internal/msg/cmd/timer.h" // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/msg/data.h" // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/msg/video_frame.h" // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/msg/audio_frame.h" // IWYU pragma: export
// 'extension.h' must be in front of 'addon.h'
#include "ten_runtime/binding/cpp/internal/extension.h" // IWYU pragma: export
// clang-format on

#include "include_internal/ten_runtime/binding/cpp/internal/extension_impl.h"  // IWYU pragma: export
#include "include_internal/ten_runtime/binding/cpp/internal/ten_env_impl.h"  // IWYU pragma: export
#include "include_internal/ten_runtime/binding/cpp/internal/ten_env_internal_accessor.h"  // IWYU pragma: export
#include "ten_runtime/addon/extension/extension.h"  // IWYU pragma: export
#include "ten_runtime/addon/extension_group/extension_group.h"  // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/addon.h"  // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/app.h"    // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/extension_group.h"  // IWYU pragma: export
#include "ten_runtime/binding/cpp/internal/ten_env_impl.h"  // IWYU pragma: export
