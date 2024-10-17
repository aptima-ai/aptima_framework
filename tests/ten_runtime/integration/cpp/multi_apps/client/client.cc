//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>

#include "ten_utils/macro/check.h"
#include "tests/common/client/cpp/msgpack_tcp.h"

namespace {

void test_extension_in_app1_not_installed() {
  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send a start_graph cmd to app 8001. However, because there is no extension
  // addon named `ext_e` in app 8001, the `start_graph` command will fail.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "type": "start_graph",
             "seq_id": "55",
             "nodes": [{
               "type": "extension",
               "name": "ext_a",
               "addon": "ext_e",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             }, {
               "type": "extension",
               "name": "ext_b",
               "addon": "ext_b",
               "app": "msgpack://127.0.0.1:8002/",
               "extension_group": "test_extension_group"
             }]
           }
         })"_json);
  TEN_ASSERT(TEN_STATUS_CODE_ERROR == resp["_ten"]["status_code"],
             "Should not happen.");

  auto detail = resp.value("detail", "");
  // NOLINTNEXTLINE
  TEN_ASSERT(!detail.empty() && detail.find("ext_e") != std::string::npos,
             "Should not happen.");

  delete client;
}

void test_extension_in_app2_not_installed() {
  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send a start_graph cmd to app 8001. However, because there is no extension
  // addon named `ext_e` in app 8002, the `start_graph` command will fail.
  auto resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "type": "start_graph",
             "seq_id": "56",
             "nodes": [{
               "type": "extension",
               "name": "ext_a",
               "addon": "ext_a",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             }, {
               "type": "extension",
               "name": "ext_b",
               "addon": "ext_e",
               "app": "msgpack://127.0.0.1:8002/",
               "extension_group": "test_extension_group"
             }],
             "connections": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group",
               "extension": "ext_a",
               "cmd": [{
                 "name": "hello_world",
                 "dest": [{
                   "app": "msgpack://127.0.0.1:8002/",
                   "extension_group": "test_extension_group",
                   "extension": "ext_b"
                 }]
               }]
             }]
           }
         })"_json);
  TEN_ASSERT(TEN_STATUS_CODE_ERROR == resp["_ten"]["status_code"],
             "Should not happen.");

  auto detail = resp.value("detail", "");
  // NOLINTNEXTLINE
  TEN_ASSERT(!detail.empty() && detail.find("ext_e") != std::string::npos,
             "Should not happen.");

  delete client;
}

}  // namespace

int main(int argc, char **argv) {
  test_extension_in_app1_not_installed();
  test_extension_in_app2_not_installed();

  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  auto resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "type": "start_graph",
             "seq_id": "55",
             "nodes": [{
               "type": "extension",
               "name": "ext_a",
               "addon": "ext_a",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group"
             }]
           }
         })"_json);
  TEN_ASSERT(TEN_STATUS_CODE_OK == resp["_ten"]["status_code"],
             "Should not happen.");

  // Send a user-defined 'hello world' command.
  resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "name": "hello_world",
             "seq_id": "137",
             "dest": [{
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test_extension_group",
               "extension": "ext_a"
             }]
           }
       })"_json);
  TEN_ASSERT(TEN_STATUS_CODE_OK == resp["_ten"]["status_code"],
             "Should not happen.");
  TEN_ASSERT(static_cast<std::string>("137") == resp["_ten"]["seq_id"],
             "Should not happen.");
  TEN_ASSERT(static_cast<std::string>("hello world, too") == resp["detail"],
             "Should not happen.");

  client->close_app();

  delete client;

  auto *client2 = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8002/");
  client2->close_app();
  delete client2;
}
