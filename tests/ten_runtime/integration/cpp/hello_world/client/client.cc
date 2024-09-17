//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>

#include "tests/common/client/cpp/msgpack_tcp.h"

int main(int argc, char **argv) {
  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8001/");

  // Send graph.
  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
           "_ten": {
             "type": "start_graph",
             "seq_id": "55",
             "nodes": [{
               "type": "extension_group",
               "name": "test extension group",
               "addon": "default_extension_group",
               "app": "msgpack://127.0.0.1:8001/"
             },{
               "type": "extension",
               "name": "test extension",
               "addon": "default_extension_cpp",
               "app": "msgpack://127.0.0.1:8001/",
               "extension_group": "test extension group"
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
               "extension_group": "test extension group",
               "extension": "test extension"
             }]
           }
       })"_json);
  TEN_ASSERT(TEN_STATUS_CODE_OK == resp["_ten"]["status_code"],
             "Should not happen.");
  TEN_ASSERT(static_cast<std::string>("137") == resp["_ten"]["seq_id"],
             "Should not happen.");
  TEN_ASSERT(static_cast<std::string>("hello world, too") == resp["detail"],
             "Should not happen.");

  delete client;
}
