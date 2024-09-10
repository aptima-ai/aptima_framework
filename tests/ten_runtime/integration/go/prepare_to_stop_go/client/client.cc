//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include <nlohmann/json.hpp>

#include "include_internal/ten_runtime/binding/cpp/ten.h"
#include "ten_utils/log/log.h"
#include "ten_utils/macro/mark.h"
#include "tests/common/client/cpp/msgpack_tcp.h"

int main(TEN_UNUSED int argc, TEN_UNUSED char **argv) {
  // Create a client and connect to the app.
  auto *client = new ten::msgpack_tcp_client_t("msgpack://127.0.0.1:8007/");

  nlohmann::json resp = client->send_json_and_recv_resp_in_json(
      R"({
    "_ten": {
      "name": "start",
      "seq_id": "238",
      "dest": [{
        "app": "msgpack://127.0.0.1:8007/",
        "graph": "0",
        "extension_group": "nodetest_group",
        "extension": "A"
      }]
    }
  })"_json);
  TEN_ASSERT(TEN_STATUS_CODE_OK == resp["_ten"]["status_code"],
             "Should not happen.");

  std::string resp_str = resp["detail"];
  TEN_LOGD("got result: %s", resp_str.c_str());
  TEN_ASSERT(resp_str == "done", "Should not happen.");

  // NOTE the order: client destroy, then connection lost, then nodejs exits
  delete client;
}
