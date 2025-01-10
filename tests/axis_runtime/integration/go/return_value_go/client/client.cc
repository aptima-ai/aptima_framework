//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <nlohmann/json.hpp>

#include "aptima_utils/macro/mark.h"
#include "tests/common/client/cpp/msgpack_tcp.h"

int main(aptima_UNUSED int argc, aptima_UNUSED char **argv) {
  // Create a client and connect to the app.
  auto *client = new aptima::msgpack_tcp_client_t("msgpack://127.0.0.1:8007/");

  auto hello_cmd = aptima::cmd_t::create("hello");
  hello_cmd->set_dest("msgpack://127.0.0.1:8007/", "default",
                      "default_extension_group", "extension_a");
  auto cmd_result = client->send_cmd_and_recv_result(std::move(hello_cmd));
  aptima_ASSERT(aptima_STATUS_CODE_OK == cmd_result->get_status_code(),
             "Should not happen.");

  std::string resp_str = cmd_result->get_property_string("detail");
  aptima_LOGD("got result: %s", resp_str.c_str());
  aptima_ASSERT(resp_str == std::string("aptima"), "Should not happen.");

  // NOTE the order: client destroy, then connection lost, then nodejs exits
  delete client;
}
