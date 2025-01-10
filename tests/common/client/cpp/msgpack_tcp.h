//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_runtime/aptima_config.h"

#include <exception>
#include <nlohmann/json.hpp>

#include "include_internal/aptima_runtime/binding/cpp/detail/msg/cmd/cmd_result_internal_accessor.h"
#include "include_internal/aptima_runtime/binding/cpp/aptima.h"
#include "include_internal/aptima_utils/log/log.h"
#include "aptima_runtime/binding/cpp/detail/msg/cmd_result.h"
#include "aptima_utils/lib/smart_ptr.h"
#include "aptima_utils/lib/string.h"
#include "tests/common/client/msgpack_tcp.h"

namespace aptima {

class msgpack_tcp_client_t {
 public:
  explicit msgpack_tcp_client_t(const char *app_id)
      : c_client(aptima_test_msgpack_tcp_client_create(app_id)) {
    if (c_client == nullptr) {
      throw std::exception();
    }
  }

  ~msgpack_tcp_client_t() { aptima_test_msgpack_tcp_client_destroy(c_client); }

  msgpack_tcp_client_t(const msgpack_tcp_client_t &) = delete;
  msgpack_tcp_client_t &operator=(const msgpack_tcp_client_t &) = delete;

  msgpack_tcp_client_t(msgpack_tcp_client_t &&) = delete;
  msgpack_tcp_client_t &operator=(msgpack_tcp_client_t &&) = delete;

  bool send_cmd(std::unique_ptr<aptima::cmd_t> &&cmd) {
    bool success = aptima_test_msgpack_tcp_client_send_msg(
        c_client, cmd->get_underlying_msg());
    if (success) {
      // Only when the cmd has been sent successfully, we should give back the
      // ownership of the cmd to the APTIMA runtime.
      auto *cpp_cmd_ptr = cmd.release();
      delete cpp_cmd_ptr;
    }
    return success;
  }

  std::unique_ptr<aptima::cmd_result_t> send_cmd_and_recv_result(
      std::unique_ptr<aptima::cmd_t> &&cmd) {
    send_cmd(std::move(cmd));

    aptima_shared_ptr_t *c_resp = aptima_test_msgpack_tcp_client_recv_msg(c_client);
    if (c_resp != nullptr) {
      return aptima::cmd_result_internal_accessor_t::create(c_resp);
    } else {
      return {};
    }
  }

  std::vector<std::unique_ptr<aptima::cmd_result_t>> batch_recv_cmd_results() {
    aptima_list_t msgs = aptima_LIST_INIT_VAL;
    aptima_test_msgpack_tcp_client_recv_msgs_batch(c_client, &msgs);

    std::vector<std::unique_ptr<aptima::cmd_result_t>> results;

    aptima_list_foreach (&msgs, iter) {
      aptima_shared_ptr_t *c_cmd_result =
          aptima_shared_ptr_clone(aptima_smart_ptr_listnode_get(iter.node));
      aptima_ASSERT(c_cmd_result, "Should not happen.");

      auto cmd_result =
          aptima::cmd_result_internal_accessor_t::create(c_cmd_result);
      results.push_back(std::move(cmd_result));
    }

    aptima_list_clear(&msgs);

    return results;
  }

  bool send_data(const std::string &graph_id,
                 const std::string &extension_group_name,
                 const std::string &extension_name, void *data, size_t size) {
    return aptima_test_msgpack_tcp_client_send_data(
        c_client, graph_id.c_str(), extension_group_name.c_str(),
        extension_name.c_str(), data, size);
  }

  bool close_app() {
    bool rc = aptima_test_msgpack_tcp_client_close_app(c_client);
    aptima_ASSERT(rc, "Should not happen.");

    return rc;
  }

  static bool close_app(const std::string &app_uri) {
    auto *client = new msgpack_tcp_client_t(app_uri.c_str());
    bool rc = aptima_test_msgpack_tcp_client_close_app(client->c_client);
    delete client;

    aptima_ASSERT(rc, "Should not happen.");
    return rc;
  }

  void get_info(std::string &ip, uint16_t &port) {
    aptima_string_t c_ip;
    aptima_string_init(&c_ip);
    aptima_test_msgpack_tcp_client_get_info(c_client, &c_ip, &port);

    ip = aptima_string_get_raw_str(&c_ip);

    aptima_string_deinit(&c_ip);
  }

 private:
  aptima_test_msgpack_tcp_client_t *c_client;
};

}  // namespace aptima
