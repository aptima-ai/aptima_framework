//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <thread>
#include <utility>

#include "gtest/gtest.h"
#include "include_internal/axis_runtime/binding/cpp/ten.h"
#include "axis_runtime/binding/cpp/detail/extension.h"
#include "axis_runtime/binding/cpp/detail/axis_env_proxy.h"
#include "axis_runtime/binding/cpp/detail/test/env_tester.h"
#include "axis_runtime/common/status_code.h"
#include "axis_utils/lang/cpp/lib/value.h"
#include "axis_utils/macro/check.h"
#include "tests/axis_runtime/smoke/util/binding/cpp/check.h"

namespace {

// This part is the extension codes written by the developer, maintained in its
// final release form, and will not change due to testing requirements.

class test_extension_1 : public ten::extension_t {
 public:
  explicit test_extension_1(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "process") {
      auto data = cmd->get_property_int64("data");
      cmd->set_property("data", data * 2);

      axis_env.send_cmd(std::move(cmd));
    } else {
      axis_ASSERT(0, "Should not happen.");
    }
  }
};

class test_extension_2 : public ten::extension_t {
 public:
  explicit test_extension_2(const char *name) : ten::extension_t(name) {}

  void on_cmd(ten::axis_env_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "process") {
      auto data = cmd->get_property_int64("data");

      auto cmd_result = ten::cmd_result_t::create(axis_STATUS_CODE_OK);
      cmd_result->set_property("data", data * data);

      axis_env.return_result(std::move(cmd_result), std::move(cmd));

      // Send another command after 1 second.
      auto *axis_env_proxy = ten::axis_env_proxy_t::create(axis_env);
      greeting_thread_ = std::thread([axis_env_proxy]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        axis_env_proxy->notify([](ten::axis_env_t &axis_env) {
          auto new_cmd = ten::cmd_t::create("hello_world");
          axis_env.send_cmd(std::move(new_cmd));
        });

        delete axis_env_proxy;
      });
    } else {
      axis_ASSERT(0, "Should not happen.");
    }
  }

  void on_stop(ten::axis_env_t &axis_env) override {
    if (greeting_thread_.joinable()) {
      greeting_thread_.join();
    }

    axis_env.on_stop_done();
  }

 private:
  std::thread greeting_thread_;
};

axis_CPP_REGISTER_ADDON_AS_EXTENSION(
    standalone_test_basic_graph__test_extension_1, test_extension_1);
axis_CPP_REGISTER_ADDON_AS_EXTENSION(
    standalone_test_basic_graph__test_extension_2, test_extension_2);

}  // namespace

namespace {

class extension_tester_1 : public ten::extension_tester_t {
 protected:
  void on_start(ten::axis_env_tester_t &axis_env) override {
    auto process_cmd = ten::cmd_t::create("process");
    process_cmd->set_property("data", 3);

    axis_env.send_cmd(
        std::move(process_cmd),
        [](ten::axis_env_tester_t & /*axis_env*/,
           std::unique_ptr<ten::cmd_result_t> result, ten::error_t *err) {
          auto data = result->get_property_int64("data");
          EXPECT_EQ(data, 36);
        });

    axis_env.on_start_done();
  }

  void on_cmd(ten::axis_env_tester_t &axis_env,
              std::unique_ptr<ten::cmd_t> cmd) override {
    if (cmd->get_name() == "hello_world") {
      axis_env.stop_test();
    }
  }
};

}  // namespace

TEST(StandaloneTest, BasicGraph) {  // NOLINT
  auto *tester = new extension_tester_1();

  // The graph is like:
  //
  // ten:test_extension -> test_extension_1 -> test_extension_2
  //        ^                                        |
  //        |                                        v
  //         ----------------------------------------
  //
  tester->set_test_mode_graph(R"({
    "nodes": [{
			"type": "extension",
			"name": "test_extension_1",
			"addon": "standalone_test_basic_graph__test_extension_1",
			"extension_group": "test_extension_group_1"
		},
		{
			"type": "extension",
			"name": "test_extension_2",
			"addon": "standalone_test_basic_graph__test_extension_2",
			"extension_group": "test_extension_group_2"
		},
		{
			"type": "extension",
			"name": "ten:test_extension",
			"addon": "ten:test_extension",
			"extension_group": "test_extension_group"
		}],
		"connections": [{
			"extension": "ten:test_extension",
			"cmd": [{
				"name": "process",
				"dest": [{
					"extension": "test_extension_1"
				}]
			}]
		},
		{
			"extension": "test_extension_1",
			"cmd": [{
				"name": "process",
				"dest": [{
					"extension": "test_extension_2"
				}]
			}]
		},
		{
			"extension": "test_extension_2",
			"cmd": [{
				"name": "hello_world",
				"dest": [{
					"extension": "ten:test_extension"
				}]
			}]
		}]})");

  bool rc = tester->run();
  axis_ASSERT(rc, "Should not happen.");

  delete tester;
}
