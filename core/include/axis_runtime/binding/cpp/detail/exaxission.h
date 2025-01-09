//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "aptima_runtime/binding/common.h"
#include "aptima_runtime/binding/cpp/detail/binding_handle.h"
#include "aptima_runtime/binding/cpp/detail/common.h"
#include "aptima_runtime/binding/cpp/detail/msg/audio_frame.h"
#include "aptima_runtime/binding/cpp/detail/msg/cmd/cmd.h"
#include "aptima_runtime/binding/cpp/detail/msg/cmd/stop_graph.h"
#include "aptima_runtime/binding/cpp/detail/msg/data.h"
#include "aptima_runtime/binding/cpp/detail/msg/video_frame.h"
#include "aptima_runtime/binding/cpp/detail/aptima_env.h"
#include "aptima_runtime/extension/extension.h"
#include "aptima_runtime/msg/cmd/stop_graph/cmd.h"
#include "aptima_runtime/msg/msg.h"
#include "aptima_runtime/aptima_env/aptima_env.h"
#include "aptima_utils/lib/json.h"
#include "aptima_utils/lib/smart_ptr.h"
#include "aptima_utils/macro/check.h"

using aptima_json_t = ::aptima_json_t;
using aptima_env_t = struct aptima_env_t;
using aptima_extension_t = struct aptima_extension_t;

namespace ten {

class aptima_env_t;
class extension_group_t;

class extension_t : public binding_handle_t {
 public:
  ~extension_t() override {
    aptima_ASSERT(get_c_instance(), "Should not happen.");
    aptima_extension_destroy(static_cast<aptima_extension_t *>(get_c_instance()));

    aptima_ASSERT(cpp_aptima_env, "Should not happen.");
    delete cpp_aptima_env;
  }

  // @{
  extension_t(const extension_t &) = delete;
  extension_t(extension_t &&) = delete;
  extension_t &operator=(const extension_t &) = delete;
  extension_t &operator=(const extension_t &&) = delete;
  // @}

 protected:
  explicit extension_t(const char *name)
      :  // In order to keep type safety in C++, the type of the 'ten'
         // parameters of these 4 functions are indeed 'ten::aptima_env_t'.
         // However, these callback functions are called from the C language
         // world, and in order to keep type safety in the C language world, it
         // better uses reinterpret_cast<> here, rather than change the type of
         // the second parameter of these callback functions from '::aptima_env_t
         // *' to 'void *'
        binding_handle_t(::aptima_extension_create(
            name,
            reinterpret_cast<aptima_extension_on_configure_func_t>(
                &proxy_on_configure),
            reinterpret_cast<aptima_extension_on_init_func_t>(&proxy_on_init),
            reinterpret_cast<aptima_extension_on_start_func_t>(&proxy_on_start),
            reinterpret_cast<aptima_extension_on_stop_func_t>(&proxy_on_stop),
            reinterpret_cast<aptima_extension_on_deinit_func_t>(&proxy_on_deinit),
            reinterpret_cast<aptima_extension_on_cmd_func_t>(&proxy_on_cmd),
            reinterpret_cast<aptima_extension_on_data_func_t>(&proxy_on_data),
            reinterpret_cast<aptima_extension_on_audio_frame_func_t>(
                &proxy_on_audio_frame),
            reinterpret_cast<aptima_extension_on_video_frame_func_t>(
                &proxy_on_video_frame),
            nullptr)) {
    aptima_ASSERT(get_c_instance(), "Should not happen.");

    aptima_binding_handle_set_me_in_target_lang(
        reinterpret_cast<aptima_binding_handle_t *>(get_c_instance()),
        static_cast<void *>(this));

    cpp_aptima_env = new aptima_env_t(aptima_extension_get_aptima_env(
        static_cast<aptima_extension_t *>(get_c_instance())));
    aptima_ASSERT(cpp_aptima_env, "Should not happen.");
  }

  virtual void on_configure(aptima_env_t &aptima_env) { aptima_env.on_configure_done(); }

  virtual void on_init(aptima_env_t &aptima_env) { aptima_env.on_init_done(); }

  virtual void on_start(aptima_env_t &aptima_env) { aptima_env.on_start_done(); }

  virtual void on_stop(aptima_env_t &aptima_env) { aptima_env.on_stop_done(); }

  virtual void on_deinit(aptima_env_t &aptima_env) { aptima_env.on_deinit_done(); }

  // Use std::shared_ptr to enable the C++ extension to save the received C++
  // messages and use it later. And use 'const shared_ptr&' to indicate that
  // the C++ extension "might" take a copy and share ownership of the cmd.

  virtual void on_cmd(aptima_env_t &aptima_env, std::unique_ptr<cmd_t> cmd) {
    auto cmd_result = ten::cmd_result_t::create(aptima_STATUS_CODE_OK);
    cmd_result->set_property("detail", "default");
    aptima_env.return_result(std::move(cmd_result), std::move(cmd));
  }

  virtual void on_data(aptima_env_t &aptima_env, std::unique_ptr<data_t> data) {}

  virtual void on_audio_frame(aptima_env_t &aptima_env,
                              std::unique_ptr<audio_frame_t> frame) {}

  virtual void on_video_frame(aptima_env_t &aptima_env,
                              std::unique_ptr<video_frame_t> frame) {}

 private:
  friend class aptima_env_t;
  friend class extension_group_t;

  using cpp_extension_on_cmd_func_t =
      void (extension_t::*)(aptima_env_t &, std::unique_ptr<cmd_t>);

  static void issue_stop_graph_cmd(aptima_env_t &aptima_env) {
    // Issue a 'close engine' command, and in order to gain the maximum
    // performance, we use C code directly here.
    aptima_shared_ptr_t *stop_graph_cmd = aptima_cmd_stop_graph_create();
    aptima_ASSERT(stop_graph_cmd, "Should not happen.");

    aptima_msg_clear_and_set_dest(stop_graph_cmd, "localhost", nullptr, nullptr,
                               nullptr, nullptr);
    aptima_env_send_cmd(aptima_env.get_c_aptima_env(), stop_graph_cmd, nullptr, nullptr,
                     nullptr);
    aptima_shared_ptr_destroy(stop_graph_cmd);
  }

  static void proxy_on_configure(aptima_extension_t *extension,
                                 ::aptima_env_t *aptima_env) {
    aptima_ASSERT(extension && aptima_env, "Should not happen.");

    auto *cpp_extension =
        static_cast<extension_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(extension)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

    cpp_extension->invoke_cpp_extension_on_configure(*cpp_aptima_env);
  }

  static void proxy_on_init(aptima_extension_t *extension, ::aptima_env_t *aptima_env) {
    aptima_ASSERT(extension && aptima_env, "Should not happen.");

    auto *cpp_extension =
        static_cast<extension_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(extension)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

    cpp_extension->invoke_cpp_extension_on_init(*cpp_aptima_env);
  }

  static void proxy_on_start(aptima_extension_t *extension, ::aptima_env_t *aptima_env) {
    aptima_ASSERT(extension && aptima_env, "Should not happen.");

    auto *cpp_extension =
        static_cast<extension_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(extension)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

    cpp_extension->invoke_cpp_extension_on_start(*cpp_aptima_env);
  }

  static void proxy_on_stop(aptima_extension_t *extension, ::aptima_env_t *aptima_env) {
    aptima_ASSERT(extension && aptima_env, "Should not happen.");

    auto *cpp_extension =
        static_cast<extension_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(extension)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

    cpp_extension->invoke_cpp_extension_on_stop(*cpp_aptima_env);
  }

  static void proxy_on_deinit(aptima_extension_t *extension,
                              ::aptima_env_t *aptima_env) {
    aptima_ASSERT(extension && aptima_env, "Should not happen.");

    auto *cpp_extension =
        static_cast<extension_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(extension)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

    cpp_extension->invoke_cpp_extension_on_deinit(*cpp_aptima_env);
  }

  static void proxy_on_cmd_internal(aptima_extension_t *extension,
                                    ::aptima_env_t *aptima_env, aptima_shared_ptr_t *cmd,
                                    cpp_extension_on_cmd_func_t on_cmd_func);

  static void proxy_on_cmd(aptima_extension_t *extension, ::aptima_env_t *aptima_env,
                           aptima_shared_ptr_t *cmd) {
    proxy_on_cmd_internal(extension, aptima_env, cmd, &extension_t::on_cmd);
  }

  static void proxy_on_data(aptima_extension_t *extension, ::aptima_env_t *aptima_env,
                            aptima_shared_ptr_t *data) {
    aptima_ASSERT(extension && aptima_env, "Should not happen.");

    auto *cpp_extension =
        static_cast<extension_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(extension)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

    auto cpp_data_ptr = std::make_unique<data_t>(
        // Clone a C shared_ptr to be owned by the C++ instance.
        aptima_shared_ptr_clone(data));

    cpp_extension->invoke_cpp_extension_on_data(*cpp_aptima_env,
                                                std::move(cpp_data_ptr));
  }

  static void proxy_on_audio_frame(aptima_extension_t *extension,
                                   ::aptima_env_t *aptima_env,
                                   aptima_shared_ptr_t *frame) {
    aptima_ASSERT(extension && aptima_env && frame, "Should not happen.");

    auto *cpp_extension =
        static_cast<extension_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(extension)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

    auto cpp_frame_ptr = std::make_unique<audio_frame_t>(
        // Clone a C shared_ptr to be owned by the C++ instance.
        aptima_shared_ptr_clone(frame));

    cpp_extension->invoke_cpp_extension_on_audio_frame(
        *cpp_aptima_env, std::move(cpp_frame_ptr));
  }

  static void proxy_on_video_frame(aptima_extension_t *extension,
                                   ::aptima_env_t *aptima_env,
                                   aptima_shared_ptr_t *frame) {
    aptima_ASSERT(extension && aptima_env && frame, "Should not happen.");

    auto *cpp_extension =
        static_cast<extension_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(extension)));
    auto *cpp_aptima_env =
        static_cast<aptima_env_t *>(aptima_binding_handle_get_me_in_target_lang(
            reinterpret_cast<aptima_binding_handle_t *>(aptima_env)));

    auto cpp_frame_ptr = std::make_unique<video_frame_t>(
        // Clone a C shared_ptr to be owned by the C++ instance.
        aptima_shared_ptr_clone(frame));

    cpp_extension->invoke_cpp_extension_on_video_frame(
        *cpp_aptima_env, std::move(cpp_frame_ptr));
  }

  void invoke_cpp_extension_on_configure(aptima_env_t &aptima_env) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      on_configure(aptima_env);
    } catch (std::exception &e) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception '") +
                                 e.what() + "' in on_configure()")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    } catch (...) {
      aptima_ENV_LOG_WARN(aptima_env,
                       (std::string("Caught an exception of type '") +
                        curr_exception_type_name() + "in on_configure().")
                           .c_str());

      issue_stop_graph_cmd(aptima_env);
    }
  }

  void invoke_cpp_extension_on_init(aptima_env_t &aptima_env) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      on_init(aptima_env);
    } catch (std::exception &e) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception '") +
                                 e.what() + "' in on_init()")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    } catch (...) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception of type '") +
                                 curr_exception_type_name() + "in on_init().")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    }
  }

  void invoke_cpp_extension_on_start(aptima_env_t &aptima_env) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      on_start(aptima_env);
    } catch (std::exception &e) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception '") +
                                 e.what() + "' in on_start()")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    } catch (...) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception of type '") +
                                 curr_exception_type_name() + "in on_start().")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    }
  }

  void invoke_cpp_extension_on_stop(aptima_env_t &aptima_env) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      on_stop(aptima_env);
    } catch (std::exception &e) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception '") +
                                 e.what() + "' in on_stop()")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    } catch (...) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception of type '") +
                                 curr_exception_type_name() + "in on_stop().")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    }
  }

  void invoke_cpp_extension_on_deinit(aptima_env_t &aptima_env) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      on_deinit(aptima_env);
    } catch (std::exception &e) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception '") +
                                 e.what() + "' in on_deinit()")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    } catch (...) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception of type '") +
                                 curr_exception_type_name() + "in on_deinit().")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    }
  }

  void invoke_cpp_extension_on_cmd(aptima_env_t &aptima_env,
                                   std::unique_ptr<cmd_t> cmd,
                                   cpp_extension_on_cmd_func_t on_cmd_func) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      (this->*on_cmd_func)(aptima_env, std::move(cmd));
    } catch (std::exception &e) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception '") +
                                 e.what() + "' in on_cmd()")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    } catch (...) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception of type '") +
                                 curr_exception_type_name() + "in on_cmd().")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    }
  }

  void invoke_cpp_extension_on_data(aptima_env_t &aptima_env,
                                    std::unique_ptr<data_t> data) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      on_data(aptima_env, std::move(data));
    } catch (std::exception &e) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception '") +
                                 e.what() + "' in on_data()")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    } catch (...) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception of type '") +
                                 curr_exception_type_name() + "in on_data().")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    }
  }

  void invoke_cpp_extension_on_audio_frame(
      aptima_env_t &aptima_env, std::unique_ptr<audio_frame_t> frame) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      on_audio_frame(aptima_env, std::move(frame));
    } catch (std::exception &e) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception '") +
                                 e.what() + "' in on_audio_frame()")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    } catch (...) {
      aptima_ENV_LOG_WARN(aptima_env,
                       (std::string("Caught an exception of type '") +
                        curr_exception_type_name() + "in on_audio_frame().")
                           .c_str());

      issue_stop_graph_cmd(aptima_env);
    }
  }

  void invoke_cpp_extension_on_video_frame(
      aptima_env_t &aptima_env, std::unique_ptr<video_frame_t> frame) {
    // The TEN runtime does not use C++ exceptions. The use of try/catch here is
    // merely to intercept any exceptions that might be thrown by the user's app
    // code. If exceptions are disabled during the compilation of the TEN
    // runtime (i.e., with -fno-exceptions), it implies that the extensions used
    // will also not employ exceptions (otherwise it would be unreasonable). In
    // this case, the try/catch blocks become no-ops. Conversely, if exceptions
    // are enabled during compilation, then the try/catch here can intercept all
    // exceptions thrown by user code that are not already caught, serving as a
    // kind of fallback.
    try {
      on_video_frame(aptima_env, std::move(frame));
    } catch (std::exception &e) {
      aptima_ENV_LOG_WARN(aptima_env, (std::string("Caught an exception '") +
                                 e.what() + "' in on_video_frame()")
                                    .c_str());

      issue_stop_graph_cmd(aptima_env);
    } catch (...) {
      aptima_ENV_LOG_WARN(aptima_env,
                       (std::string("Caught an exception of type '") +
                        curr_exception_type_name() + "in on_video_frame().")
                           .c_str());

      issue_stop_graph_cmd(aptima_env);
    }
  }

  aptima_env_t *cpp_aptima_env;
};

}  // namespace ten
