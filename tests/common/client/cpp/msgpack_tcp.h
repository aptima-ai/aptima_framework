//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <exception>
#include <nlohmann/json.hpp>

#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/lib/string.h"
#include "tests/common/client/msgpack_tcp.h"

namespace ten {

class msgpack_tcp_client_t {
 public:
  explicit msgpack_tcp_client_t(const char *app_id)
      : c_client(ten_test_msgpack_tcp_client_create(app_id)) {
    if (c_client == nullptr) {
      throw std::exception();
    }
  }

  ~msgpack_tcp_client_t() { ten_test_msgpack_tcp_client_destroy(c_client); }

  msgpack_tcp_client_t(const msgpack_tcp_client_t &) = delete;
  msgpack_tcp_client_t &operator=(const msgpack_tcp_client_t &) = delete;

  msgpack_tcp_client_t(msgpack_tcp_client_t &&) = delete;
  msgpack_tcp_client_t &operator=(msgpack_tcp_client_t &&) = delete;

  bool send_cmd(std::unique_ptr<ten::cmd_t> &&cmd) {
    bool success = ten_test_msgpack_tcp_client_send_msg(
        c_client, cmd->get_underlying_msg());
    if (success) {
      // Only when the cmd has been sent successfully, we should give back the
      // ownership of the cmd to the TEN runtime.
      auto *cpp_cmd_ptr = cmd.release();
      delete cpp_cmd_ptr;
    }
    return success;
  }

  bool send_json(const nlohmann::json &cmd_json) {
    if (cmd_json.contains("_ten")) {
      if (cmd_json["_ten"].contains("type")) {
        if (cmd_json["_ten"]["type"] == "start_graph") {
          std::unique_ptr<cmd_t> cmd = ten::cmd_start_graph_t::create();
          cmd->from_json(cmd_json.dump().c_str());
          return send_cmd(std::move(cmd));
        } else if (cmd_json["_ten"]["type"] == "close_app") {
          std::unique_ptr<ten::cmd_t> cmd = ten::cmd_close_app_t::create();
          cmd->from_json(cmd_json.dump().c_str());
          return send_cmd(std::move(cmd));
        } else {
          TEN_ASSERT(0, "Handle more TEN builtin command type.");
        }
      } else {
        std::unique_ptr<ten::cmd_t> cmd = ten::cmd_t::create(
            cmd_json["_ten"]["name"].get<std::string>().c_str());
        cmd->from_json(cmd_json.dump().c_str());
        return send_cmd(std::move(cmd));
      }
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }

    return false;
  }

  nlohmann::json send_cmd_and_recv_resp_in_json(
      std::unique_ptr<ten::cmd_t> &&cmd) {
    send_cmd(std::move(cmd));

    ten_shared_ptr_t *c_resp = ten_test_msgpack_tcp_client_recv_msg(c_client);
    if (c_resp != nullptr) {
      ten_json_t *c_json = ten_msg_to_json(c_resp, nullptr);

      bool must_free = false;
      const char *json_str = ten_json_to_string(c_json, nullptr, &must_free);
      TEN_ASSERT(json_str, "Failed to get JSON string from JSON.");

      nlohmann::json result = nlohmann::json::parse(json_str);

      ten_json_destroy(c_json);
      if (must_free) {
        TEN_FREE(
            json_str);  // NOLINT(cppcoreguidelines-no-malloc, hicpp-no-malloc)
      }

      ten_shared_ptr_destroy(c_resp);

      return result;
    } else {
      return nlohmann::json{};
    }
  }

  nlohmann::json send_json_and_recv_resp_in_json(
      const nlohmann::json &cmd_json) {
    if (cmd_json.contains("_ten")) {
      if (cmd_json["_ten"].contains("type")) {
        if (cmd_json["_ten"]["type"] == "start_graph") {
          std::unique_ptr<ten::cmd_start_graph_t> start_graph_cmd =
              ten::cmd_start_graph_t::create();
          start_graph_cmd->from_json(cmd_json.dump().c_str());
          return send_cmd_and_recv_resp_in_json(std::move(start_graph_cmd));
        } else {
          TEN_ASSERT(0, "Handle more TEN builtin command type.");
        }
      } else {
        std::unique_ptr<ten::cmd_t> custom_cmd = ten::cmd_t::create(
            cmd_json["_ten"]["name"].get<std::string>().c_str());
        custom_cmd->from_json(cmd_json.dump().c_str());
        return send_cmd_and_recv_resp_in_json(std::move(custom_cmd));
      }
    } else {
      TEN_ASSERT(0, "Should not happen.");
    }
    return {};
  }

  std::vector<nlohmann::json> batch_recv_resp_in_json() {
    ten_list_t msgs = TEN_LIST_INIT_VAL;

    ten_test_msgpack_tcp_client_recv_msgs_batch(c_client, &msgs);

    std::vector<nlohmann::json> results;

    ten_list_foreach (&msgs, iter) {
      ten_shared_ptr_t *c_resp = ten_smart_ptr_listnode_get(iter.node);
      TEN_ASSERT(c_resp, "Should not happen.");

      ten_json_t *c_json = ten_msg_to_json(c_resp, nullptr);

      bool must_free = false;
      const char *json_str = ten_json_to_string(c_json, nullptr, &must_free);
      TEN_ASSERT(json_str, "Failed to get JSON string from JSON.");

      results.emplace_back(nlohmann::json::parse(json_str));

      ten_json_destroy(c_json);
      if (must_free) {
        TEN_FREE(
            json_str);  // NOLINT(cppcoreguidelines-no-malloc, hicpp-no-malloc)
      }
    }

    ten_list_clear(&msgs);

    return results;
  }

  bool send_data(const std::string &graph_name,
                 const std::string &extension_group_name,
                 const std::string &extension_name, void *data, size_t size) {
    return ten_test_msgpack_tcp_client_send_data(
        c_client, graph_name.c_str(), extension_group_name.c_str(),
        extension_name.c_str(), data, size);
  }

  bool close_app() {
    bool rc = ten_test_msgpack_tcp_client_close_app(c_client);
    TEN_ASSERT(rc, "Should not happen.");

    return rc;
  }

  static bool close_app(const std::string &app_uri) {
    auto *client = new msgpack_tcp_client_t(app_uri.c_str());
    bool rc = ten_test_msgpack_tcp_client_close_app(client->c_client);
    delete client;

    TEN_ASSERT(rc, "Should not happen.");
    return rc;
  }

  void get_info(std::string &ip, uint16_t &port) {
    ten_string_t c_ip;
    ten_string_init(&c_ip);
    ten_test_msgpack_tcp_client_get_info(c_client, &c_ip, &port);

    ip = ten_string_get_raw_str(&c_ip);

    ten_string_deinit(&c_ip);
  }

 private:
  ten_test_msgpack_tcp_client_t *c_client;
};

}  // namespace ten
