---
hidden: true
layout:
  title:
    visible: true
  description:
    visible: false
  tableOfContents:
    visible: true
  outline:
    visible: true
  pagination:
    visible: true
---

# 🚧 How to build extension with C++(beta)

## Overview

This tutorial introduces how to develop an TEN extension using C++, as well as how to debug and deploy it to run in an TEN app. This tutorial covers the following topics:

* How to create a C++ extension development project using tman.
* How to use TEN API to implement the functionality of the extension, such as sending and receiving messages.
* How to write unit test cases and debug the code.
* How to deploy the extension locally to an app and perform integration testing within the app.
* How to debug the extension code within the app.

Note

Unless otherwise specified, the commands and code in this tutorial are executed in a Linux environment. Since TEN has a consistent development approach and logic across all platforms (e.g., Windows, Mac), this tutorial is also suitable for other platforms.

### Preparation

* Download the latest tman and configure the PATH. You can check if it is configured correctly with the following command:

    ```
    $ tman -h
    ```

    If the configuration is successful, it will display the help information for tman as follows:

    ```
    TEN manager

    Usage: tman [OPTIONS] <COMMAND>

    Commands:
      install     Install a package. For more detailed usage, run 'install -h'
      uninstall   Uninstall a package. For more detailed usage, run 'uninstall -h'
      package     Create a package file. For more detailed usage, run 'package -h'
      publish     Publish a package. For more detailed usage, run 'publish -h'
      dev-server  Install a package. For more detailed usage, run 'dev-server -h'
      help        Print this message or the help of the given subcommand(s)

    Options:
      -c, --config-file <CONFIG_FILE>  The location of config.json
          --user-token <USER_TOKEN>    The user token
          --verbose                    Enable verbose output
      -h, --help                       Print help
      -V, --version                    Print version
    ```

* Download the latest ten\_gn and configure the PATH. For example:

    Note

    ten\_gn is the C++ build system for the TEN platform. To facilitate developers, TEN provides a ten\_gn toolchain for building C++ extension projects.

    ```
    $ export PATH=/path/to/ten_gn:$PATH
    ```

    You can check if the configuration is successful with the following command:

    ```
    $ tgn -h
    ```

    usage: tgn [-h] [-v] [--verbose] [--out-dir OUT_DIR] command target-OS target-CPU build-type

    An easy-to-use Google gn wrapper

    positional arguments:
      command            possible commands are:
                        gen         build        rebuild            refs    clean
                        graph       uninstall    explain_build      desc    check
                        show_deps   show_input   show_input_output  path    args
      target-OS          possible OS values are:
                        win   mac   linux
      target-CPU         possible values are:
                        x86   x64   arm   arm64
      build-type         possible values are:
                        debug   release

    options:
      -h, --help         show this help message and exit
      -v, --version      show program's version number and exit
      --verbose          dump verbose outputs
      --out-dir OUT_DIR  build output dir, default is 'out/'

    I recommend you to put /usr/local/ten_gn/.gnfiles into your PATH so that you can run tgn anywhere.
    ```

    Note

  * gn depends on python3, please make sure that Python 3.10 or above is installed.
* Install a C/C++ compiler, either clang/clang++ or gcc/g++.

In addition, we provide a base compilation image where all of the above dependencies are already installed and configured. You can refer to the [TEN-Agent](https://github.com/ten-framework/TEN-Agent) project on GitHub.

### Creating C++ extension project

#### Creating Based on Templates

Assuming we want to create a project named first\_cxx\_extension, we can use the following command to create it:

```
$ tman install extension default_extension_cpp --template-mode --template-data package_name=first_cxx_extension
```

Note

The above command indicates that we are installing an TEN package using the default\_extension\_cpp template to create an extension project named first\_cxx\_extension.

* \--template-mode indicates installing the TEN package as a template. The template rendering parameters can be specified using --template-data.
* extension is the type of TEN package to install. Currently, TEN provides app/extension\_group/extension/system packages. In the following sections on testing extensions in an app, we will use several other types of packages.
* default\_extension\_cpp is the default C++ extension provided by TEN. Developers can also specify other C++ extensions available in the store as templates.

After the command is executed, a directory named first\_cxx\_extension will be generated in the current directory, which is our C++ extension project. The directory structure is as follows:

```
.
├── BUILD.gn
├── manifest.json
├── property.json
└── src
  └── main.cc
```

Where:

* src/main.cc contains a simple implementation of the extension, including calls to the C++ API provided by TEN. We will discuss how to use the TEN API in the next section.
* manifest.json and property.json are the standard configuration files for TEN extensions. In manifest.json, metadata information such as the version, dependencies, and schema definition of the extension are typically declared. property.json is used to declare the business configuration of the extension.
* BUILD.gn is the configuration file for ten\_gn, used to compile the C++ extension project.

The property.json file is initially an empty JSON file, like this:

```
{}
```

The manifest.json file will include the ten\_runtime dependency by default, like this:

```
{
  "type": "extension",
  "name": "first_cxx_extension",
  "version": "0.2.0",
  "dependencies": [
  {
    "type": "system",
    "name": "ten_runtime",
    "version": "0.2.0"
  }
  ],
  "api": {}
}
```

Note

* Please note that according to TEN's naming convention, the name should be alphanumeric. This is because when integrating the extension into an app, a directory will be created based on the extension name. TEN also provides the functionality to automatically load the manifest.json and property.json files from the extension directory.
* Dependencies are used to declare the dependencies of the extension. When installing TEN packages, tman will automatically download the dependencies based on the declarations in the dependencies section.
* The api section is used to declare the schema of the extension. Refer to `usage of ten schema <usage_of_ten_schema_cn>`.

#### Manual Creation

Developers can also manually create a C++ extension project or transform an existing project into an TEN extension project.

First, ensure that the project's output target is a shared library. Then, refer to the example above to create `property.json` and `manifest.json` in the project's root directory. The `manifest.json` should include information such as `type`, `name`, `version`, `language`, and `dependencies`. Specifically:

* `type` must be `extension`.
* `language` must be `cpp`.
* `dependencies` should include `ten_runtime`.

Finally, configure the build settings. The `default_extension_cpp` provided by TEN uses `ten_gn` as the build toolchain. If developers are using a different build toolchain, they can refer to the configuration in `BUILD.gn` to set the compilation parameters. Since `BUILD.gn` contains the directory structure of the TEN package, we will discuss it in the next section (Downloading Dependencies).

### Download Dependencies

To download dependencies, execute the following command in the extension project directory:

```
$ tman install
```

After the command is executed successfully, a `.ten` directory will be generated in the current directory, which contains all the dependencies of the current extension.

Note

* There are two modes for extensions: development mode and runtime mode. In development mode, the root directory is the source code directory of the extension. In runtime mode, the root directory is the app directory. Therefore, the placement path of dependencies is different in these two modes. The `.ten` directory mentioned here is the root directory of dependencies in development mode.

The directory structure is as follows:

```
.
├── BUILD.gn
├── manifest.json
├── property.json
├── .ten
│   └── app
│       ├── addon
│       ├── include
│       └── lib
└── src
  └── main.cc
```

Where:

* `.ten/app/include` is the root directory for header files.
* `.ten/app/lib` is the root directory for precompiled dynamic libraries of TEN runtime.

If it is in runtime mode, the extension will be placed in the `addon/extension` directory of the app, and the dynamic libraries will be placed in the `lib` directory of the app. The structure is as follows:

```
.
├── BUILD.gn
├── manifest.json
├── property.json
├── addon
│   └── extension
│       └── first_cxx_extension
├── include
└── lib
```

So far, an TEN C++ extension project has been created.

### BUILD.gn

The content of `BUILD.gn` for `default_extension_cpp` is as follows:

```
import("//exts/ten/base_options.gni")
import("//exts/ten/ten_package.gni")

config("common_config") {
  defines = common_defines
  include_dirs = common_includes
  cflags = common_cflags
  cflags_c = common_cflags_c
  cflags_cc = common_cflags_cc
  cflags_objc = common_cflags_objc
  cflags_objcc = common_cflags_objcc
  libs = common_libs
  lib_dirs = common_lib_dirs
  ldflags = common_ldflags
}

config("build_config") {
  configs = [ ":common_config" ]

  # 1. The `include` refers to the `include` directory in current extension.
  # 2. The `//include` refers to the `include` directory in the base directory
  #    of running `tgn gen`.
  # 3. The `.ten/app/include` is used in extension standalone building.
  include_dirs = [
  "include",
  "//core/include",
  "//include/nlohmann_json",
  ".ten/app/include",
  ".ten/app/include/nlohmann_json",
  ]

  lib_dirs = [
  "lib",
  "//lib",
  ".ten/app/lib",
  ]

  if (is_win) {
  libs = [
    "ten_runtime.dll.lib",
    "utils.dll.lib",
  ]
  } else {
  libs = [
    "ten_runtime",
    "utils",
  ]
  }
}

ten_package("first_cxx_extension") {
  package_type = "develop"  # develop | release
  package_kind = "extension"

  manifest = "manifest.json"
  property = "property.json"

  if (package_type == "develop") {
  # It's 'develop' package, therefore, need to build the result.
  build_type = "shared_library"

  sources = [ "src/main.cc" ]

  configs = [ ":build_config" ]
  }
}
```

Let's first take a look at the `ten_package` target, which declares a build target for an TEN package.

* The `package_kind` is set to `extension`, and the `build_type` is set to `shared_library`. This means that the expected output of the compilation is a shared library.
* The `sources` field specifies the source file(s) to be compiled. If there are multiple source files, they need to be added to the `sources` field.
* The `configs` field specifies the build configurations. It references the `build_config` defined in this file.

Next, let's look at the content of `build_config`.

* The `include_dirs` field defines the search paths for header files.
  * The difference between `include` and `//include` is that `include` refers to the `include` directory in the current extension directory, while `//include` is based on the working directory of the `tgn gen` command. So, if the compilation is executed in the extension directory, it will be the same as `include`. But if it is executed in the app directory, it will be the `include` directory in the app.
  * `.ten/app/include` is used for standalone development and compilation of the extension, which is the scenario being discussed in this tutorial. In other words, the default `build_config` is compatible with both development mode and runtime mode compilation.
* The `lib_dirs` field defines the search paths for dependency libraries. The difference between `lib` and `//lib` is similar to `include`.
* The `libs` field defines the dependent libraries. `ten_runtime` and `utils` are libraries provided by TEN.

Therefore, if developers are using a different build toolchain, they can refer to the above configuration and set the compilation parameters in their own build toolchain. For example, if using g++ to compile:

```
$ g++ -shared -fPIC -I.ten/app/include/ -L.ten/app/lib -lten_runtime -lutils -Wl,-rpath=\$ORIGIN -Wl,-rpath=\$ORIGIN/../../../lib src/main.cc
```

The setting of `rpath` is also considered for the runtime mode, where the ten\_runtime dependency of the extension is placed in the `app/lib` directory.

### Implementation of Extension Functionality

For developers, there are two things to do:

* Create an extension as a channel for interacting with TEN runtime.
* Register the extension as an addon in TEN, allowing it to be used in the graph through a declarative approach.

#### Creating the Extension Class

The extension created by developers needs to inherit the `ten::extension_t` class. The main definition of this class is as follows:

```
class extension_t {
protected:
  explicit extension_t(const std::string &name) {...}

  virtual void on_init(ten_t &ten, metadata_info_t &manifest,
                     metadata_info_t &property) {
    ten.on_init_done(manifest, property);
  }

  virtual void on_start(ten_t &ten) { ten.on_start_done(); }

  virtual void on_stop(ten_t &ten) { ten.on_stop_done(); }

  virtual void on_deinit(ten_t &ten) { ten.on_deinit_done(); }

  virtual void on_cmd(ten_t &ten, std::unique_ptr<cmd_t> cmd) {
    auto cmd_result = ten::cmd_result_t::create(TEN_STATUS_CODE_OK);
    cmd_result->set_property("detail", "default");
    ten.return_result(std::move(cmd_result), std::move(cmd));
  }

  virtual void on_data(ten_t &ten, std::unique_ptr<data_t> data) {}

  virtual void on_pcm_frame(ten_t &ten, std::unique_ptr<pcm_frame_t> frame) {}

  virtual void on_image_frame(ten_t &ten,
                              std::unique_ptr<image_frame_t> frame) {}
}
```

In the markdown content you provided, there are descriptions of the lifecycle functions and message handling functions in Chinese. Here is the translation:

Lifecycle Functions:

* on\_init: Used to initialize the extension instance, such as setting the extension's configuration.
* on\_start: Used to start the extension instance, such as establishing connections to external services. The extension will not receive messages until on\_start is completed. In on\_start, you can use the ten.get\_property API to retrieve the extension's configuration.
* on\_stop: Used to stop the extension instance, such as closing connections to external services.
* on\_deinit: Used to destroy the extension instance, such as releasing memory resources.

Message Handling Functions:

* on\_cmd/on\_data/on\_pcm\_frame/on\_image\_frame: These are callback methods used to receive messages of four different types. For more information on TEN message types, you can refer to the [message-system](https://github.com/TEN-framework/ten_framework/blob/main/docs/ten_framework/message_system.md)

The ten::extension\_t class provides default implementations for these functions, and developers can override them according to their needs.

#### Registering the Extension

After defining the extension, it needs to be registered as an addon in the TEN runtime. For example, in the `first_cxx_extension/src/main.cc` file, the registration code is as follows:

```
TEN_CPP_REGISTER_ADDON_AS_EXTENSION(first_cxx_extension, first_cxx_extension_extension_t);
```

* TEN\_CPP\_REGISTER\_ADDON\_AS\_EXTENSION is a macro provided by the TEN runtime for registering extension addons.
  * The first parameter is the name of the addon, which serves as a unique identifier for the addon. It will be used to define the extension in the graph using a declarative approach.
  * The second parameter is the implementation class of the extension, which is the class that inherits from ten::extension\_t.

Please note that the addon name must be unique because it is used as a unique index to find the implementation in the graph.

#### on\_init

Developers can set the extension's configuration in the on\_init() function, as shown in the example:

```
void on_init(ten::ten_t& ten, ten::metadata_info_t& manifest,
             ten::metadata_info_t& property) override {
  property.set(TEN_METADATA_JSON_FILENAME, "customized_property.json");
  ten.on_init_done(manifest, property);
}
```

Both the property and manifest can be customized using the set() method. In the example, the first parameter TEN\_METADATA\_JSON\_FILENAME indicates that the custom property is stored as a local file, and the second parameter is the file path relative to the extension directory. So in this example, when the app loads the extension, it will load `<app>/addon/extension/first_cxx_extension/customized_property.json`.

TEN's on\_init provides default logic for loading default configurations. If developers do not call property.set(), the property.json file in the extension directory will be loaded by default. Similarly, if manifest.set() is not called, the manifest.json file in the extension directory will be loaded by default. In the example, since property.set() is called, the property.json file will not be loaded by default.

Please note that on\_init is an asynchronous method, and developers need to call ten.on\_init\_done() to inform the TEN runtime that on\_init has completed as expected.

#### on\_start

When on\_start is called, it means that on\_init\_done() has been executed and the extension's property has been loaded. From this point on, the extension can access the configuration. For example:

```
void on_start(ten::ten_t& ten) override {
  auto prop = ten.get_property_string("some_string");
  // do something

  ten.on_start_done();
}
```

ten.get\_property\_string() is used to retrieve a property of type string with the name "some\_string". If the property does not exist or the type does not match, an error will be returned. If the extension's configuration contains the following content:

```
{
  "some_string": "hello world"
}
```

Then the value of prop will be "hello world".

Similar to on\_init, on\_start is also an asynchronous method, and developers need to call ten.on\_start\_done() to inform the TEN runtime that on\_start has completed as expected.

For more information, you can refer to the API documentation: ten api doc.

#### Error Handling

As shown in the previous example, if "some\_string" does not exist or is not of type string, ten.get\_property\_string() will return an error. You can handle the error as follows:

```C++
void on_start(ten::ten_t& ten) override {
  ten::error_t err;
  auto prop = ten.get_property_string("some_string", &err);

  // error handling
  if (!err.is_success()) {
    TEN_LOGE("Failed to get property: %s", err.errmsg());
  }

  ten.on_start_done();
}
```

#### Message Handling

TEN provides four types of messages: `cmd`, `data`, `image_frame`, and `pcm_frame`. Developers can handle these four types of messages by implementing the `on_cmd`, `on_data`, `on_image_frame`, and `on_pcm_frame` callback methods.

Taking `cmd` as an example, let's see how to receive and send messages.

Assume that `first_cxx_extension` receives a `cmd` with the name `hello`, which includes the following properties:

| name             | type   |
| ---------------- | ------ |
| app\_id          | string |
| client\_type     | int8   |
| payload          | object |
| payload.err\_no  | uint8  |
| payload.err\_msg | string |

The processing logic of `first_cxx_extension` for the `hello` cmd is as follows:

* If the `app_id` or `client_type` parameters are invalid, return an error:

    ```json
    {
      "err_no": 1001,
      "err_msg": "Invalid argument."
    }
    ```

* If `payload.err_no` is greater than 0, return an error with the content from the `payload`.
* If `payload.err_no` is equal to 0, forward the `hello` cmd downstream for further processing. After receiving the processing result from the downstream extension, return the result.

**Describing the Extension's Behavior in manifest.json**

Based on the above description, the behavior of `first_cxx_extension` is as follows:

* It receives a `cmd` named `hello` with properties.
* It may send a `cmd` named `hello` with properties.
* It receives a response from a downstream extension, which includes error information.
* It returns a response to an upstream extension, which includes error information.

For a TEN extension, you can describe the above behavior in the `manifest.json` file of the extension, including:

* What messages the extension receives, their names, and the structure definition of their properties (schema definition).
* What messages the extension generates/sends, their names, and the structure definition of their properties.
* Additionally, for `cmd` type messages, a response definition is required (referred to as a result in TEN).

With these definitions, TEN runtime will perform validity checks based on the schema definition before delivering messages to the extension or when the extension sends messages through TEN runtime. It also helps the users of the extension to see the protocol definition.

The schema is defined in the `api` field of the `manifest.json` file. `cmd_in` defines the cmds that the extension will receive, and `cmd_out` defines the cmds that the extension will send.

Note

For the usage of schema, refer to: [TEN Framework Schema System](https://github.com/TEN-framework/ten_framework/blob/main/docs/ten_framework/schema_system.md)

Based on the above description, the content of `manifest.json` for `first_cxx_extension` is as follows:

```
{
  "type": "extension",
  "name": "first_cxx_extension",
  "version": "0.2.0",
  "dependencies": [
    {
      "type": "system",
      "name": "ten_runtime",
      "version": "0.2.0"
    }
  ],
  "api": {
    "cmd_in": [
      {
        "name": "hello",
        "property": {
          "app_id": {
            "type": "string"
          },
          "client_type": {
            "type": "int8"
          },
          "payload": {
            "type": "object",
            "properties": {
              "err_no": {
                "type": "uint8"
              },
              "err_msg": {
                "type": "string"
              }
            }
          }
        },
        "required": ["app_id", "client_type"],
        "result": {
          "property": {
            "err_no": {
              "type": "uint8"
            },
            "err_msg": {
              "type": "string"
            }
          },
          "required": ["err_no"]
        }
      }
    ],
    "cmd_out": [
      {
        "name": "hello",
        "property": {
          "app_id": {
            "type": "string"
          },
          "client_type": {
            "type": "string"
          },
          "payload": {
            "type": "object",
            "properties": {
              "err_no": {
                "type": "uint8"
              },
              "err_msg": {
                "type": "string"
              }
            }
          }
        },
        "required": ["app_id", "client_type"],
        "result": {
          "property": {
            "err_no": {
              "type": "uint8"
            },
            "err_msg": {
              "type": "string"
            }
          },
          "required": ["err_no"]
        }
      }
    ]
  }
}
```

**Getting Request Data**

In the `on_cmd` method, the first step is to retrieve the request data, which is the property in the cmd. We define a `request_t` class to represent the request data.

Create a file called `model.h` in the `include` directory of your extension project with the following content:

```
#pragma once

#include "nlohmann/json.hpp"
#include <cstdint>
#include <string>

namespace first_cxx_extension_extension {

class request_payload_t {
public:
  friend void from_json(const nlohmann::json &j, request_payload_t &payload);

  friend class request_t;

private:
  uint8_t err_no;
  std::string err_msg;
};

class request_t {
public:
  friend void from_json(const nlohmann::json &j, request_t &request);

private:
  std::string app_id;
  int8_t client_type;
  request_payload_t payload;
};

} // namespace first_cxx_extension_extension
```

In the `src` directory, create a file called `model.cc` with the following content:

```
#include "model.h"

namespace first_cxx_extension_extension {
void from_json(const nlohmann::json &j, request_payload_t &payload) {
  if (j.contains("err_no")) {
    j.at("err_no").get_to(payload.err_no);
  }

  if (j.contains("err_msg")) {
    j.at("err_msg").get_to(payload.err_msg);
  }
}

void from_json(const nlohmann::json &j, request_t &request) {
  if (j.contains("app_id")) {
    j.at("app_id").get_to(request.app_id);
  }

  if (j.contains("client_type")) {
    j.at("client_type").get_to(request.client_type);
  }

  if (j.contains("payload")) {
    j.at("payload").get_to(request.payload);
  }
}
} // namespace first_cxx_extension_extension
```

To parse the request data, you can use the `get_property` API provided by TEN. Here is an example of how to implement it:

```
// model.h

class request_t {
public:
  void from_cmd(ten::cmd_t &cmd);

  // ...
}

// model.cc

void request_t::from_cmd(ten::cmd_t &cmd) {
  app_id = cmd.get_property_string("app_id");
  client_type = cmd.get_property_int8("client_type");

  auto payload_str = cmd.get_property_to_json("payload");
  if (!payload_str.empty()) {
    auto payload_json = nlohmann::json::parse(payload_str);
    from_json(payload_json, payload);
  }
}
```

To return a response, you need to create a `cmd_result_t` object and set the properties accordingly. Then, pass the `cmd_result_t` object to TEN runtime to return it to the requester. Here is an example:

```
// model.h

class request_t {
public:
  bool validate(std::string *err_msg) {
    if (app_id.length() < 64) {
      *err_msg = "invalid app_id";
      return false;
    }

    return true;
  }
}

// main.cc

void on_cmd(ten::ten_t &ten, std::unique_ptr<ten::cmd_t> cmd) override {
  request_t request;
  request.from_cmd(*cmd);

  std::string err_msg;
  if (!request.validate(&err_msg)) {
    auto result = ten::cmd_result_t::create(TEN_STATUS_CODE_ERROR);
    result->set_property("err_no", 1);
    result->set_property("err_msg", err_msg.c_str());

    ten.return_result(std::move(result), std::move(cmd));
  }
}
```

In the example above, `ten::cmd_result_t::create` is used to create a `cmd_result_t` object with an error code. `result.set_property` is used to set the properties of the `cmd_result_t` object. Finally, `ten.return_result` is called to return the `cmd_result_t` object to the requester.

**Passing Requests to Downstream Extensions**

If an extension needs to send a message to another extension, it can call the `send_cmd()` API. Here is an example:

```
void on_cmd(ten::ten_t &ten, std::unique_ptr<ten::cmd_t> cmd) override {
  request_t request;
  request.from_cmd(*cmd);

  std::string err_msg;
  if (!request.validate(&err_msg)) {
    // ...
  } else {
    ten.send_cmd(std::move(cmd));
  }
}
```

The first parameter in `send_cmd()` is the command of the request, and the second parameter is the handler for the returned `cmd_result_t`. The second parameter can also be omitted, indicating that no special handling is required for the returned result. If the command was originally sent from a higher-level extension, the runtime will automatically return it to the upper-level extension.

Developers can also pass a response handler, like this:

```
ten.send_cmd(
    std::move(cmd),
    [](ten::ten_t &ten, std::unique_ptr<ten::cmd_result_t> result) {
      ten.return_result_directly(std::move(result));
    });
```

In the example above, the `return_result_directly()` method is used in the response handler. You can see that this method differs from `return_result()` in that it does not pass the original command object. This is mainly because:

* For TEN message objects (cmd/data/pcm\_frame/image\_frame), ownership is transferred to the extension in the message callback method, such as `on_cmd()`. This means that once the extension receives the command, the TEN runtime will not perform any read/write operations on it. When the extension calls the `send_cmd()` or `return_result()` API, it means that the extension is returning the ownership of the command back to the TEN runtime for further processing, such as message delivery. After that, the extension should not perform any read/write operations on the command.
* The `result` in the response handler (i.e., the second parameter of `send_cmd()`) is returned by the downstream extension, and at this point, the result is already bound to the command, meaning that the runtime has the return path information for the result. Therefore, there is no need to pass the command object again.

Of course, developers can also process the result in the response handler.

So far, an example of a simple command processing logic is complete. For other message types such as data, you can refer to the TEN API documentation.

### Deploying Locally to an App for Integration Testing

tman provides the ability to publish to a local registry, allowing you to perform integration testing locally without uploading the extension to the central repository. Unlike GO extensions, for C++ extensions, there are no strict requirements on the app's programming language. It can be GO, C++, or Python.

The deployment process may vary for different apps. The specific steps are as follows:

* Set up the tman local registry.
* Upload the extension to the local registry.
* Download the app from the central repository (default\_app\_cpp/default\_app\_go) for integration testing.
* For C++ apps:
  * Install the first\_cxx\_extension in the app directory.
  * Compile in the app directory. At this point, both the app and the extension will be compiled into the out/linux/x64/app/default\_app\_cpp directory.
  * Install the required dependencies in out/linux/x64/app/default\_app\_cpp. The working directory for testing is the current directory.
* For GO apps:
  * Install the first\_cxx\_extension in the app directory.
  * Compile in the addon/extension/first\_cxx\_extension directory, as the GO and C++ compilation toolchains are different.
  * Install the dependencies in the app directory. The working directory for testing is the app directory.
* Configure the graph in the app's manifest.json, specifying the recipient of the message as first\_cxx\_extension, and send test messages.

#### Uploading the Extension to the Local Registry

First, create a temporary config.json file to set up the tman local registry. For example, the contents of /tmp/code/config.json are as follows:

```
{
  "registry": {
    "default": {
      "index": "file:///tmp/code/repository"
    }
  }
}
```

This sets the local directory `/tmp/code/repository` as the tman local registry.

Note

* Be careful not to place it in \~/.tman/config.json, as it will affect the subsequent download of dependencies from the central repository.

Then, in the first\_cxx\_extension directory, execute the following command to upload the extension to the local registry:

```
$ tman --config-file /tmp/code/config.json publish
```

After the command completes, the uploaded extension can be found in the /tmp/code/repository/extension/first\_cxx\_extension/0.1.0 directory.

#### Prepare app for testing (C++)

1. Install default\_app\_cpp as the test app in an empty directory.

> ```
> $ tman install app default_app_cpp
> ```
>
> After the command is successfully executed, there will be a directory named default\_app\_cpp in the current directory.
>
> Note
>
> * When installing an app, its dependencies will be automatically installed.

2. Install first\_cxx\_extension that we want to test in the app directory.

> Execute the following command:
>
> ```
> $ tman --config-file /tmp/code/config.json install extension first_cxx_extension
> ```
>
> After the command is completed, there will be a first\_cxx\_extension directory in the addon/extension directory.
>
> Note
>
> * It is important to note that since first\_cxx\_extension is in the local registry, the configuration file path with the local registry specified by --config-file needs to be the same as when publishing.

3. Add an extension as a message producer.

> first\_cxx\_extension is expected to receive a hello cmd, so we need a message producer. One way is to add an extension as a message producer. To conveniently generate test messages, an http server can be integrated into the producer's extension.
>
> First, create an http server extension based on default\_extension\_cpp. Execute the following command in the app directory:
>
> ```
> $ tman install extension default_extension_cpp --template-mode --template-data package_name=http_server
> ```
>
> The main functionality of the http server is:
>
> * Start a thread running the http server in the extension's on\_start().
> * Convert incoming requests into TEN cmds named hello and send them using send\_cmd().
> * Expect to receive a cmd\_result\_t response and write its content to the http response.
>
> Here, we use cpp-httplib ([https://github.com/yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib)) as the implementation of the http server.
>
> First, download httplib.h and place it in the include directory of the extension. Then, add the implementation of the http server in src/main.cc. Here is an example code:
>
> ```
> #include "httplib.h"
> #include "nlohmann/json.hpp"
> #include "ten_runtime/binding/cpp/ten.h"
>
> namespace http_server_extension {
>
> class http_server_extension_t : public ten::extension_t {
> public:
>   explicit http_server_extension_t(const std::string &name)
>       : extension_t(name) {}
>
>   void on_start(ten::ten_t &ten) override {
>     ten_proxy = ten::ten_proxy_t::create(ten);
>     srv_thread = std::thread([this] {
>       server.Get("/health",
>                 [](const httplib::Request &req, httplib::Response &res) {
>                   res.set_content("OK", "text/plain");
>                 });
>
>       // Post handler, receive json body.
>       server.Post("/hello", [this](const httplib::Request &req,
>                                   httplib::Response &res) {
>         // Receive json body.
>         auto body = nlohmann::json::parse(req.body);
>         body["ten"]["name"] = "hello";
>
>         auto cmd = ten::cmd_t::create_from_json(body.dump().c_str());
>         auto cmd_shared =
>             std::make_shared<std::unique_ptr<ten::cmd_t>>(std::move(cmd));
>
>         std::condition_variable *cv = new std::condition_variable();
>
>         auto response_body = std::make_shared<std::string>();
>
>         ten_proxy->notify([cmd_shared, response_body, cv](ten::ten_t &ten) {
>           ten.send_cmd(
>               std::move(*cmd_shared),
>               [response_body, cv](ten::ten_t &ten,
>                                   std::unique_ptr<ten::cmd_result_t> result) {
>                 auto err_no = result->get_property_uint8("err_no");
>                 if (err_no > 0) {
>                   auto err_msg = result->get_property_string("err_msg");
>                   response_body->append(err_msg);
>                 } else {
>                   response_body->append("OK");
>                 }
>
>                 cv->notify_one();
>               });
>         });
>
>         std::unique_lock<std::mutex> lk(mtx);
>         cv->wait(lk);
>         delete cv;
>
>         res.set_content(response_body->c_str(), "text/plain");
>       });
>
>       server.listen("0.0.0.0", 8001);
>     });
>
>     ten.on_start_done();
>   }
>
>   void on_stop(ten::ten_t &ten) override {
>     // Extension stop.
>
>     server.stop();
>     srv_thread.join();
>     delete ten_proxy;
>
>     ten.on_stop_done();
>   }
>
> private:
>   httplib::Server server;
>   std::thread srv_thread;
>   ten::ten_proxy_t *ten_proxy{nullptr};
>   std::mutex mtx;
> };
>
> TEN_CPP_REGISTER_ADDON_AS_EXTENSION(http_server, http_server_extension_t);
>
> } // namespace http_server_extension
> ```

Here, a new thread is created in `on_start()` to run the http server because we don't want to block the extension thread. This way, the convetend cmd requests are generated and sent from `srv_thread`. In the TEN runtime, to ensure thread safety, we use `ten_proxy_t` to pass calls like `send_cmd()` from threads outside the extension thread.

This code also demonstrates how to clean up external resources in `on_stop()`. For an extension, you should release the `ten_proxy_t` before `on_stop_done()`, which stops the external thread.

1. Configure the graph.

In the app's `manifest.json`, configure `predefined_graph` to specify that the `hello` cmd generated by `http_server` should be sent to `first_cxx_extension`. For example:

> ```
> "predefined_graphs": [
>   {
>     "name": "testing",
>     "auto_start": true,
>     "nodes": [
>       {
>         "type": "extension_group",
>         "name": "http_thread",
>         "addon": "default_extension_group"
>       },
>       {
>         "type": "extension",
>         "name": "http_server",
>         "addon": "http_server",
>         "extension_group": "http_thread"
>       },
>       {
>         "type": "extension",
>         "name": "first_cxx_extension",
>         "addon": "first_cxx_extension",
>         "extension_group": "http_thread"
>       }
>     ],
>     "connections": [
>       {
>         "extension_group": "http_thread",
>         "extension": "http_server",
>         "cmd": [
>           {
>             "name": "hello",
>             "dest": [
>               {
>                 "extension_group": "http_thread",
>                 "extension": "first_cxx_extension"
>               }
>             ]
>           }
>         ]
>       }
>     ]
>   }
> ]
> ```

5. Compile the app.

> Execute the following commands in the app directory:
>
> ```
> $ tgn gen linux x64 debug
> $ tgn build linux x64 debug
> ```
>
> After the compilation is complete, the compilation output for the app and extension will be generated in the directory out/linux/x64/app/default\_app\_cpp.
>
> However, it cannot be run directly at this point as it is missing the dependencies of the extension group.

6. Install the extension group.

> Switch to the compilation output directory.
>
> ```
> $ cd out/linux/x64/app/default_app_cpp
> ```
>
> Install the extension group.
>
> ```
> $ tman install extension_group default_extension_group
> ```

7. Start the app.

> In the compilation output directory, execute the following command:
>
> ```
> $ ./bin/default_app_cpp
> ```
>
> After the app starts, you can now test it by sending messages to the http server. For example, use curl to send a request with an invalid app\_id:
>
> ```
> $ curl --location 'http://127.0.0.1:8001/hello' \
>   --header 'Content-Type: application/json' \
>   --data '{
>       "app_id": "123",
>       "client_type": 1,
>       "payload": {
>           "err_no": 0
>       }
>   }'
> ```
>
> The expected response should be "invalid app\_id".

### Debugging extension in an app

#### App (C++)

A C++ app is compiled into an executable file with the correct `rpath` set. Therefore, debugging a C++ app only requires adding the following configuration to `.vscode/launch.json`:

```
"configurations": [
  {
      "name": "App (C/C++) (lldb, launch)",
      "type": "lldb",
      "request": "launch",
      "program": "${workspaceFolder}/out/linux/x64/app/default_app_cpp/bin/default_app_cpp",
      "args": [],
      "cwd": "${workspaceFolder}/out/linux/x64/app/default_app_cpp"
  }
  ]
```
