# For Users of the APTIMA Framework

## Standalone Testing of Extension

The APTIMA framework provides an independent extension testing mechanism. This mechanism allows developers to test extensions without relying on other APTIMA concepts, such as other APTIMA extensions, APTIMA graphs, or APTIMA apps. When developers need to test the behavior of an extension without running the entire APTIMA app, this independent extension testing mechanism is especially useful.

The standalone testing framework for APTIMA extensions follows two key principles:

1. Compatibility with any native testing framework of the used language.

   For example, if a APTIMA extension is developed using C++, the Google gtest/gmock testing framework can be used alongside the APTIMA extension standalone testing framework to achieve independent testing of the C++ APTIMA extension.

2. Users don't need to modify any code in the extension under test.

   The exact same extension code used in runtime can be fully tested using this standalone testing framework.

3. The standalone testing flow for extensions developed in different languages follows the same design principles and usage methods.

   This ensures that once you learn the standalone testing concepts and methods for one language, you can quickly grasp the standalone testing concepts and methods for APTIMA extensions in other languages.

For users, the APTIMA extension standalone testing framework introduces two main concepts:

1. **extension_tester**
2. **ten_env_tester**

The role of **extension_tester** is similar to a testing driver, responsible for setting up and executing the entire testing process. **ten_env_tester**, on the other hand, behaves like a typical APTIMA extension's `ten_env` instance, enabling users to invoke functionalities within the standalone testing framework from the callback of `extension_tester`, such as sending messages to the extension under test and receiving messages from the extension.

From the API design of **extension_tester** and **ten_env_tester**, you can see that they are very similar to **APTIMA extension** and **ten_env**, and this is by design. The main purpose is to allow users familiar with extension development to quickly get accustomed to the standalone testing framework and then develop standalone test cases for the extension itself.

However, for testing purposes, there are inevitably APIs and features dedicated specifically to testing. To prevent these test-specific functionalities and APIs, which are not needed during actual runtime, from polluting the runtime API set, the standalone testing framework is designed not to directly use or extend the API sets of **extension** and **ten_env**. Instead, it introduces types and API sets that are exclusive to standalone testing. This separation ensures that the types and API sets used in actual runtime and those used during testing do not interfere with each other, avoiding any negative side effects.

## Standalone Testing Framework Internal

Internally, the APTIMA extension standalone testing framework implicitly starts a test app, loads the extension addon to be tested (let’s call it extension A), and initializes a graph containing both the extension A to be tested and another extension (let’s call it extension B) dedicated to testing other extensions. All input and output messages of extension A are redirected to extension B, allowing users to customize inputs and outputs during the testing process and complete the actual testing work.

```mermaid
sequenceDiagram
  participant tester as Extension Tester
  participant testing as Extension Testing
  participant tested as Extension Tested

  tester->>testing: send messages
  testing->>tested: send messages

  tested-->>testing: send results
  testing-->>tester: send results
```

Basically, you can think of this testing extension hidden within the testing framework as a proxy between the extension being tested and the tester. It acts as a proxy extension within the APTIMA graph, facilitating the exchange of messages between the tested extension and the tester using the language of the APTIMA environment.

## Basic Testing Process

The basic testing process and logic are as follows:

1. Create an extension tester to manage the entire standalone testing process.
2. Inform the standalone testing framework of the folder containing the extension to be tested.
3. Set a testing mode, such as a mode for testing a single extension.
4. Start the testing.

## C++

Here is an example of APTIMA extension standalone testing using Google gtest:

```c++
class extension_tester_basic : public aptima::extension_tester_t {
 public:
  void on_start(aptima::ten_env_tester_t &ten_env) override {
    auto new_cmd = aptima::cmd_t::create("hello_world");
    ten_env.send_cmd(std::move(new_cmd),
                     [](aptima::ten_env_tester_t &ten_env,
                        std::unique_ptr<aptima::cmd_result_t> result) {
                       if (result->get_status_code() == TEN_STATUS_CODE_OK) {
                         ten_env.stop_test();
                       }
                     });
  }
};

TEST(Test, Basic) {
  // 1. Create an extension tester to manage the entire standalone testing process.
  auto *tester = new extension_tester_basic();

  // 2. Inform the standalone testing framework of the folder containing the extension to be tested.
  ten_string_t *path = ten_path_get_executable_path();
  ten_path_join_c_str(path, "../ten_packages/extension/default_extension_cpp/");

  tester->add_addon_base_dir(ten_string_get_raw_str(path));

  ten_string_destroy(path);

  // 3. Set a testing mode, such as a mode for testing a single extension.
  tester->set_test_mode_single("default_extension_cpp");

  // 4. Start the testing.
  tester->run();

  delete tester;
}
```

## Golang

TODO: To be added.

## Python

```python
class ExtensionTesterBasic(ExtensionTester):
    def check_hello(self, ten_env: TenEnvTester, result: CmdResult):
        statusCode = result.get_status_code()
        print("receive hello_world, status:" + str(statusCode))

        if statusCode == StatusCode.OK:
            ten_env.stop_test()

    def on_start(self, ten_env: TenEnvTester) -> None:
        new_cmd = Cmd.create("hello_world")

        print("send hello_world")
        ten_env.send_cmd(
            new_cmd,
            lambda ten_env, result: self.check_hello(ten_env, result),
        )

        print("tester on_start_done")
        ten_env.on_start_done()


def test_basic():
    # 1. Create an extension tester to manage the entire standalone testing process.
    tester = ExtensionTesterBasic()

    # 2. Inform the standalone testing framework of the folder containing the extension to be tested.
    tester.add_addon_base_dir(str(Path(__file__).resolve().parent.parent))

    # 3. Set a testing mode, such as a mode for testing a single extension.
    tester.set_test_mode_single("default_extension_python")

    # 4. Start the testing.
    tester.run()
```
