# TEN Manager - Check Graph

`tman` provides the `check graph` command to validate predefined graphs or a `start_graph` command for correctness. To see the usage details, use the following command:

```shell
$ tman check graph -h
Check whether the graph content of the predefined graph or start_graph command is correct. For more detailed usage, run 'graph -h'

Usage: tman check graph [OPTIONS] --app <APP>

Options:
      --app <APP>
          The absolute path of the app defined in the graph. By default, the predefined graph will be read from the first one in the list.
      --predefined-graph-name <PREDEFINED_GRAPH_NAME>
          Specify a predefined graph name to check, otherwise, all predefined graphs will be checked.
      --graph <GRAPH>
          Specify the JSON string of a 'start_graph' command to be checked. If not specified, the predefined graph in the first app will be checked.
  -h, --help
          Print help
```

The `check graph` command is designed to handle graphs that may span multiple apps, so it does not require being run from the root directory of a specific app. Instead, it can be executed from any directory, with the `--app` parameter used to specify the folders of the apps involved in the graph.

## Example Usages

- **Check all predefined graphs in `property.json`**:

  ```shell
  tman check graph --app /home/TEN-Agent/agents
  ```

- **Check a specific predefined graph**:

  ```shell
  tman check graph --predefined-graph-name va.openai.azure --app /home/TEN-Agent/agents
  ```

- **Check a `start_graph` command**:

  ```shell
  tman check graph --graph '{
    "type": "start_graph",
    "seq_id": "55",
    "nodes": [
      {
        "type": "extension",
        "name": "test_extension",
        "addon": "basic_hello_world_2__test_extension",
        "extension_group": "test_extension_group",
        "app": "msgpack://127.0.0.1:8001/"
      },
      {
        "type": "extension",
        "name": "test_extension",
        "addon": "basic_hello_world_1__test_extension",
        "extension_group": "test_extension_group",
        "app": "msgpack://127.0.0.1:8001/"
      }
    ]
  }' --app /home/TEN-Agent/agents
  ```

## Prerequisites

- **Predefined Graph Definition Requirement**: If a predefined graph name is specified, the definition will be extracted from the `property.json` file of the app specified by the first `--app` parameter.

- **Package Installation Requirement**: Before running the `check graph` command, all extensions that the app depends on must be installed using the `tman install` command. This is necessary because the validation process requires information about each extension in the graph, such as APIs defined in their `manifest.json` files.

- **Unique App URI Requirement**: In a multi-app graph, each app's `property.json` must define a unique `_ten::uri`. Additionally, the `uri` value cannot be set to `"localhost"`.

## Validation Rules

### 1. Presence of Nodes

The `nodes` array is required in any graph definition. If absent, an error will be thrown.

- **Example (No nodes defined)**:

  ```shell
  tman check graph --graph '{
    "type": "start_graph",
    "seq_id": "55",
    "nodes": []
  }' --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  Checking graph[0]... ❌. Details:
    No extension node is defined in graph.

  All is done.
  💔  Error: 1/1 graphs failed.
  ```

### 2. Uniqueness of Nodes

Each node in the `nodes` array represents a specific extension instance within a group of an app, created by a specified addon. Therefore, each extension instance should be uniquely represented by a single node. A node must be uniquely identified by the combination of `app`, `extension_group`, and `name`. Multiple entries for the same extension instance are not allowed.

- **Example (Duplicate nodes)**:

  ```shell
  tman check graph --graph '{
    "type": "start_graph",
    "seq_id": "55",
    "nodes": [
      {
        "type": "extension",
        "name": "test_extension",
        "addon": "basic_hello_world_2__test_extension",
        "extension_group": "test_extension_group",
        "app": "msgpack://127.0.0.1:8001/"
      },
      {
        "type": "extension",
        "name": "test_extension",
        "addon": "basic_hello_world_1__test_extension",
        "extension_group": "test_extension_group",
        "app": "msgpack://127.0.0.1:8001/"
      }
    ]
  }' --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  Checking graph[0]... ❌. Details:
    Duplicated extension was found in nodes[1], addon: basic_hello_world_1__test_extension, name: test_extension.

  All is done.
  💔  Error: 1/1 graphs failed.
  ```

### 3. Extensions used in connections should be defined in nodes

All extension instances referenced in the `connections` field, whether as a source or destination, must be explicitly defined in the `nodes` field. Any instance not defined in the `nodes` array will cause validation errors.

- **Example (Source extension not defined)**:

  Imagine that the content of `property.json` of a TEN app is as follows.

  ```json
  {
    "_ten": {
      "predefined_graphs": [
        {
          "name": "default",
          "auto_start": false,
          "nodes": [
            {
              "type": "extension",
              "name": "some_extension",
              "addon": "default_extension_go",
              "extension_group": "some_group"
            }
          ],
          "connections": [
            {
              "extension": "some_extension",
              "extension_group": "producer",
              "cmd": [
                {
                  "name": "hello",
                  "dest": [
                    {
                      "extension_group": "some_group",
                      "extension": "some_extension"
                    }
                  ]
                }
              ]
            }
          ]
        }
      ]
    }
  }
  ```

  ```shell
  tman check graph --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  Checking graph[0]... ❌. Details:
    The extension declared in connections[0] is not defined in nodes, extension_group: producer, extension: some_extension.

  All is done.
  💔  Error: 1/1 graphs failed.
  ```

- **Example (Destination extension not defined)**:

  Imagine that the content of `property.json` of a TEN app is as follows.

  ```json
  {
    "_ten": {
      "predefined_graphs": [
        {
          "name": "default",
          "auto_start": false,
          "nodes": [
            {
              "type": "extension",
              "name": "some_extension",
              "addon": "default_extension_go",
              "extension_group": "some_group"
            },
            {
              "type": "extension",
              "name": "some_extension_1",
              "addon": "default_extension_go",
              "extension_group": "some_group"
            }
          ],
          "connections": [
            {
              "extension": "some_extension",
              "extension_group": "some_group",
              "cmd": [
                {
                  "name": "hello",
                  "dest": [
                    {
                      "extension_group": "some_group",
                      "extension": "some_extension_1"
                    }
                  ]
                },
                {
                  "name": "world",
                  "dest": [
                    {
                      "extension_group": "some_group",
                      "extension": "some_extension_1"
                    },
                    {
                      "extension_group": "some_group",
                      "extension": "consumer"
                    }
                  ]
                }
              ]
            }
          ]
        }
      ]
    }
  }
  ```

  ```shell
  tman check graph --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  Checking graph[0]... ❌. Details:
    The extension declared in connections[0].cmd[1] is not defined in nodes extension_group: some_group, extension: consumer.

  All is done.
  💔  Error: 1/1 graphs failed.
  ```

### 4. The addons declared in the `nodes` must be installed in the app

- **Example (The `_ten::uri` in property.json is not equal to the `app` field in nodes)**:

  Imagine that the content of property.json of a TEN app is as follows.

  ```json
  {
    "_ten": {
      "predefined_graphs": [
        {
          "name": "default",
          "auto_start": false,
          "nodes": [
            {
              "type": "extension",
              "name": "some_extension",
              "addon": "default_extension_go",
              "extension_group": "some_group"
            }
          ]
        }
      ],
      "uri": "http://localhost:8001"
    }
  }
  ```

  ```shell
  tman check graph --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  Checking graph[0]... ❌. Details:
    The following packages are declared in nodes but not installed: [("localhost", Extension, "default_extension_go")].

  All is done.
  💔  Error: 1/1 graphs failed.
  ```

  The problem is all packages in the app will be stored in a map which key is the `uri` of the app, and each node in the graph is retrieved by the `app` field (which is `localhost` by default). The `app` in node (i.e., localhost) is mismatch with the `uri` of app (i.e., <http://localhost:8001>).

- **Example (the ten_packages does not exist as the `tman install` has not executed)**:

  Imagine that the content of property.json of a TEN app is as follows.

  ```json
  {
    "_ten": {
      "predefined_graphs": [
        {
          "name": "default",
          "auto_start": false,
          "nodes": [
            {
              "type": "extension",
              "name": "some_extension",
              "addon": "default_extension_go",
              "extension_group": "some_group"
            }
          ]
        }
      ]
    }
  }
  ```

  And the `ten_packages` directory does **\_NOT** exist.

  ```shell
  tman check graph --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  Checking graph[0]... ❌. Details:
    The following packages are declared in nodes but not installed: [("localhost", Extension, "default_extension_go")].

  All is done.
  💔  Error: 1/1 graphs failed.
  ```

### 5. In connections, messages sent from one extension should be defined in the same section

- **Example**:

  Imagine that all the packages have been installed.

  ```shell
  tman check graph --graph '{
    "nodes": [
      {
        "type": "extension",
        "name": "some_extension",
        "addon": "default_extension_go",
        "extension_group": "some_group"
      },
      {
        "type": "extension",
        "name": "another_ext",
        "addon": "default_extension_go",
        "extension_group": "some_group"
      }
    ],
    "connections": [
      {
        "extension": "some_extension",
        "extension_group": "some_group",
        "cmd": [
          {
            "name": "hello",
            "dest": [
              {
                "extension_group": "some_group",
                "extension": "another_ext"
              }
            ]
          }
        ]
      },
      {
        "extension": "some_extension",
        "extension_group": "some_group",
        "cmd": [
          {
            "name": "hello_2",
            "dest": [
              {
                "extension_group": "some_group",
                "extension": "another_ext"
              }
            ]
          }
        ]
      }
    ]
  }' --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  Checking graph[0]... ❌. Details:
    extension 'some_extension' is defined in connection[0] and connection[1], merge them into one section.

  All is done.
  💔  Error: 1/1 graphs failed.
  ```

### 6. In connections, the messages sent out from one extension should have a unique name in each type

- **Example**:

  Imagine that all the packages have been installed.

  ```shell
  tman check graph --graph '{
    "nodes": [
      {
        "type": "extension",
        "name": "some_extension",
        "addon": "addon_a",
        "extension_group": "some_group"
      },
      {
        "type": "extension",
        "name": "another_ext",
        "addon": "addon_b",
        "extension_group": "some_group"
      }
    ],
    "connections": [
      {
        "extension": "some_extension",
        "extension_group": "some_group",
        "cmd": [
          {
            "name": "hello",
            "dest": [
              {
                "extension_group": "some_group",
                "extension": "another_ext"
              }
            ]
          },
          {
            "name": "hello",
            "dest": [
              {
                "extension_group": "some_group",
                "extension": "some_extension"
              }
            ]
          }
        ]
      }
    ]
  }' --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  Checking graph[0]... ❌. Details:
    - connection[0]:
     - Merge the following cmd into one section:
        'hello' is defined in flow[0] and flow[1].

  All is done.
  💔  Error: 1/1 graphs failed.
  ```

### 7. The messages declared in the connections should be compatible

The message declared in each message flow in the connections will be checked if the schema is compatible, according to the schema definition in the manifest.json of extensions. The rules of message compatible are as follows.

- If the message schema is not found neither in the source extension nor the target extension, the message is compatible.
- If the message schema is only found in one of the extensions, the message is incompatible.
- The message is compatible only if the following conditions are met.

  - The property type is compatible.
    - If the property is an object, the fields both in the source and target schema must have a compatible type.
    - If the `required` keyword is defined in the target schema, there must be a `required` keyword in the source schema, which is the superset of the `required` in the target.

- **Example**:

  Imagine that all the packages have been installed.

  The content of manifest.json in `addon_a` is as follows.

  ```json
  {
    "type": "extension",
    "name": "addon_a",
    "version": "0.1.0",
    "dependencies": [
      {
        "type": "system",
        "name": "ten_runtime_go",
        "version": "0.1.0"
      }
    ],
    "package": {
      "include": ["**"]
    },
    "api": {
      "cmd_out": [
        {
          "name": "cmd_1",
          "property": {
            "foo": {
              "type": "string"
            }
          }
        }
      ]
    }
  }
  ```

  And, the content of manifest.json in `addon_b` is as follows.

  ```json
  {
    "type": "extension",
    "name": "addon_b",
    "version": "0.1.0",
    "dependencies": [
      {
        "type": "system",
        "name": "ten_runtime_go",
        "version": "0.1.0"
      }
    ],
    "package": {
      "include": ["**"]
    },
    "api": {
      "cmd_in": [
        {
          "name": "cmd_1",
          "property": {
            "foo": {
              "type": "int8"
            }
          }
        }
      ]
    }
  }
  ```

  Checking graph with the following command.

  ```shell
  tman check graph --graph '{
    "nodes": [
      {
        "type": "extension",
        "name": "some_extension",
        "addon": "addon_a",
        "extension_group": "some_group"
      },
      {
        "type": "extension",
        "name": "another_ext",
        "addon": "addon_b",
        "extension_group": "some_group"
      }
    ],
    "connections": [
      {
        "extension": "some_extension",
        "extension_group": "some_group",
        "cmd": [
          {
            "name": "cmd_1",
            "dest": [
              {
                "extension_group": "some_group",
                "extension": "another_ext"
              }
            ]
          }
        ]
      }
    ]
  }' --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  Checking graph[0]... ❌. Details:
    - connections[0]:
      - cmd[0]:  Schema incompatible to [extension_group: some_group, extension: another_ext], properties are incompatible:
      property [foo], Type is incompatible, source is [string], but target is [int8].

  All is done.
  💔 Error: 1/1 graphs failed.
  ```

### 8. The `app` in node must be unambiguous

The `app` field in each node must met the following rules.

- The `app` field must be equal to the `_ten::uri` of the corresponding TEN app.
- Either all nodes should have `app` declared, or none should.
- The `app` field can not be `localhost`.
- The `app` field can not be an empty string.

- **Example (some of the nodes specified the `app` field)**:

  Checking graph with the following command.

  ```shell
  tman check graph --graph '{
    "nodes": [
      {
        "type": "extension",
        "name": "some_extension",
        "addon": "addon_a",
        "extension_group": "some_group"
      },
      {
        "type": "extension",
        "name": "another_ext",
        "addon": "addon_b",
        "extension_group": "some_group",
        "app": "http://localhost:8000"
      }
    ]
  }' --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  💔  Error: The graph json string is invalid

  Caused by:
    Either all nodes should have 'app' declared, or none should, but not a mix of both.
  ```

- **Example (No `app` specified in all nodes, but some source extension specified `app` in the connections)**:

  Checking graph with the following command.

  ```shell
  tman check graph --graph '{
    "nodes": [
      {
        "type": "extension",
        "name": "some_extension",
        "addon": "addon_a",
        "extension_group": "some_group"
      },
      {
        "type": "extension",
        "name": "another_ext",
        "addon": "addon_b",
        "extension_group": "some_group"
      }
    ],
    "connections": [
     {
        "extension": "some_extension",
        "extension_group": "some_group",
        "app": "http://localhost:8000",
        "cmd": [
          {
            "name": "cmd_1",
            "dest": [
              {
                "extension_group": "some_group",
                "extension": "another_ext"
              }
            ]
          }
        ]
      }
    ]
  }' --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  💔  Error: The graph json string is invalid

  Caused by:
    connections[0].the 'app' should not be declared, as not any node has declared it
  ```

- **Example (No `app` specified in all nodes, but some target extension specified `app` in the connections)**:

  Checking graph with the following command.

  ```shell
  tman check graph --graph '{
    "nodes": [
      {
        "type": "extension",
        "name": "some_extension",
        "addon": "addon_a",
        "extension_group": "some_group"
      },
      {
        "type": "extension",
        "name": "another_ext",
        "addon": "addon_b",
        "extension_group": "some_group"
      }
    ],
    "connections": [
      {
        "extension": "some_extension",
        "extension_group": "some_group",
        "cmd": [
          {
            "name": "cmd_1",
            "dest": [
              {
                "extension_group": "some_group",
                "extension": "another_ext",
                "app": "http://localhost:8000"
              }
            ]
          }
        ]
      }
    ]
  }' --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  💔  Error: The graph json string is invalid

  Caused by:
    connections[0].cmd[0].dest[0]: the 'app' should not be declared, as not any node has declared it
  ```

- **Example (The `app` field in nodes is not equal to the `_ten::uri` of app)**:

  Same as [Rule 4](#4-the-addons-declared-in-the-nodes-must-be-installed-in-the-app).

- **Example (The `app` field is `localhost` in a single-app graph)**:

  Checking graph with the following command.

  ```shell
  tman check graph --graph '{
    "nodes": [
      {
        "type": "extension",
        "name": "some_extension",
        "addon": "addon_a",
        "extension_group": "some_group"
      },
      {
        "type": "extension",
        "name": "another_ext",
        "addon": "addon_b",
        "extension_group": "some_group",
        "app": "localhost"
      }
    ],
    "connections": [
      {
        "extension": "some_extension",
        "extension_group": "some_group",
        "cmd": [
          {
            "name": "cmd_1",
            "dest": [
              {
                "extension_group": "some_group",
                "extension": "another_ext"
              }
            ]
          }
        ]
      }
    ]
  }' --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  💔  Error: Failed to parse graph string, nodes[1]: 'localhost' is not allowed in graph definition, and the graph seems to be a single-app graph, just remove the 'app' field
  ```

- **Example (The `app` field is `localhost` in a multi-app graph)**:

  Checking graph with the following command.

  ```shell
  tman check graph --graph '{
    "nodes": [
      {
        "type": "extension",
        "name": "some_extension",
        "addon": "addon_a",
        "extension_group": "some_group",
        "app": "http://localhost:8000"
      },
      {
        "type": "extension",
        "name": "another_ext",
        "addon": "addon_b",
        "extension_group": "some_group",
        "app": "localhost"
      }
    ],
    "connections": [
      {
        "extension": "some_extension",
        "extension_group": "some_group",
        "app": "http://localhost:8000",
        "cmd": [
          {
            "name": "cmd_1",
            "dest": [
              {
                "extension_group": "some_group",
                "extension": "another_ext"
              }
            ]
          }
        ]
      }
    ]
  }' --app /home/TEN-Agent/agents
  ```

  **Output**:

  ```text
  💔  Error: Failed to parse graph string, nodes[1]: 'localhost' is not allowed in graph definition, change the content of 'app' field to be consistent with '_ten::uri'
  ```
