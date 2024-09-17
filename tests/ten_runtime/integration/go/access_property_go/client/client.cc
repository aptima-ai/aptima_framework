//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <nlohmann/json.hpp>

#include "ten_utils/macro/mark.h"
#include "tests/common/client/cpp/msgpack_tcp.h"

int main(TEN_UNUSED int argc, TEN_UNUSED char **argv) {
  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8007/");

  nlohmann::json resp = client->send_json_and_recv_resp_in_json(R"({
    "_ten": {
        "name": "test",
        "seq_id": "111",
        "dest": [{
          "app": "msgpack://127.0.0.1:8007/",
          "graph": "0",
          "extension_group": "nodetest",
          "extension": "A"
        }]
      }
    })"_json);
  TEN_ASSERT(TEN_STATUS_CODE_OK == resp["_ten"]["status_code"],
             "Should not happen.");

  TEN_LOGD("Got graph result.");

  std::string resp_str = resp["detail"];
  TEN_LOGD("Got result: %s", resp_str.c_str());
  TEN_ASSERT(resp_str == std::string("okok"), "Should not happen.");

  // NOTE the order: client destroy, then connection lost, then nodejs exits.
  delete client;
}
