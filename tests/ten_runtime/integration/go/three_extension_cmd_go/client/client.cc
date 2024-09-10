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

  nlohmann::json resp = client->send_json_and_recv_resp_in_json(R"({
    "_ten": {
      "type": "start_graph",
      "seq_id": "156",
      "nodes": [
        {
          "type": "extension_group",
          "app": "msgpack://127.0.0.1:8007/",
          "addon": "default_extension_group",
          "name": "nodetest_group"
        },
        {
          "type": "extension",
          "app": "msgpack://127.0.0.1:8007/",
          "extension_group": "nodetest_group",
          "addon": "nodetest",
          "name": "A"
        },
        {
          "type": "extension",
          "app": "msgpack://127.0.0.1:8007/",
          "extension_group": "nodetest_group",
          "addon": "nodetest",
          "name": "B"
        },
        {
          "type": "extension",
          "app": "msgpack://127.0.0.1:8007/",
          "extension_group": "nodetest_group",
          "addon": "nodetest",
          "name": "C"
        }
      ],
      "connections": [
        {
          "app": "msgpack://127.0.0.1:8007/",
          "extension_group": "nodetest_group",
          "extension": "A",
          "cmd": [{
            "name": "B",
            "dest": [{
              "app": "msgpack://127.0.0.1:8007/",
              "extension_group": "nodetest_group",
              "extension": "B"
            }]
          }]
        },
        {
          "app": "msgpack://127.0.0.1:8007/",
          "extension_group": "nodetest_group",
          "extension": "B",
          "cmd": [{
            "name": "C",
            "dest": [{
              "app": "msgpack://127.0.0.1:8007/",
              "extension_group": "nodetest_group",
              "extension": "C"
            }]
          }]
        }
      ]
    }
  })"_json);
  TEN_LOGD("client sent json");

  TEN_ASSERT(TEN_STATUS_CODE_OK == resp["_ten"]["status_code"],
             "Should not happen.");

  TEN_LOGD("got graph result");

  resp = client->send_json_and_recv_resp_in_json(
      R"({
    "_ten": {
      "name": "A",
      "seq_id": "238",
      "dest": [{
        "app": "msgpack://127.0.0.1:8007/",
        "extension_group": "nodetest_group",
        "extension": "A"
      }]
    }
  })"_json);
  TEN_ASSERT(TEN_STATUS_CODE_OK == resp["_ten"]["status_code"],
             "Should not happen.");

  std::string resp_str = resp["detail"];
  TEN_LOGD("got result: %s", resp_str.c_str());
  ten_json_t *result = ten_json_from_string(resp_str.c_str(), NULL);
  TEN_ASSERT(
      30 == ten_json_get_number_value(ten_json_object_peek(result, "result")),
      "Should not happen.");
  ten_json_destroy(result);

  // NOTE the order: client destroy, then connection lost, then nodejs exits
  delete client;
}
