# TEN Manager - Dev Server

To start the `tman` development server, use the following command:

{% code title=">_ Terminal" %}

```shell
tman dev-server
```

{% endcode %}

If the `base-dir` is not specified, the current working directory will be used by default. Regardless, `base-dir` must be the base directory of a TEN app.

The server starts on port 49483 by default, and you can interact with the dev-server using the following URL:

{% code title="https" %}

```text
http://127.0.0.1:49483/api/dev-server/v1/
```

{% endcode %}

If the requested endpoint URL is not found, the client will receive a `404 Not Found` response, with the response body containing `Endpoint '\' not found` to prevent any confusion.

## Version

Retrieve the version of the dev-server.

- **Endpoint:** `/api/dev-server/v1/version`
- **Verb:** GET

You will receive a `200 OK` response, with the body containing a JSON object like this:

{% code title=".json" %}

```json
{
  "version": "0.1.0"
}
```

{% endcode %}

## Installed Extension Addons

Retrieve all installed extension addons recognized by the dev-server under the base directory.

- **Endpoint:** `/api/dev-server/v1/addons/extensions`
- **Verb:** GET

You will receive a `200 OK` response, with the body containing a JSON array like this:

{% code title=".json" %}

```json
[
  {
    "name": "default_extension_cpp"
  },
  {
    "name": "simple_http_server_cpp"
  }
]
```

{% endcode %}

## Available Graphs

Retrieve a list of available graphs.

- **Endpoint:** `/api/dev-server/v1/graphs`
- **Verb:** GET

You will receive a `200 OK` response, with the body containing a JSON array like this:

{% code title=".json" %}

```json
[
  {
    "auto_start": true,
    "name": "0"
  }
]
```

{% endcode %}

If an error occurs, such as when the App package is not found, you will receive a `400 Bad Request` response with the body containing `Failed to find any app packages`.

## Extensions in a Specified Graph

Retrieve the list of extensions within a specified graph.

- **Endpoint:** `/api/dev-server/v1/graphs/{graph_id}/nodes`
- **Verb:** GET

You will receive a `200 OK` response, with the body containing a JSON array. Ex:

{% code title=".json" %}

```json
{
  "status": "ok",
  "data": [
    {
      "addon": "addon_a",
      "name": "ext_a",
      "extension_group": "some_group",
      "app": "localhost",
      "api": {
        "property": {
          "foo": {
            "type": "string"
          }
        },
        "cmd_in": [
          {
            "name": "hello",
            "property": [
              {
                "name": "foo",
                "attributes": {
                  "type": "string"
                }
              }
            ],
            "result": {
              "property": [
                {
                  "name": "detail",
                  "attributes": {
                    "type": "string"
                  }
                }
              ]
            }
          }
        ],
        "data_out": [
          {
            "name": "hello",
            "property": [
              {
                "name": "bar",
                "attributes": {
                  "type": "Uint8"
                }
              }
            ]
          }
        ]
      },
      "property": {
        "foo": "1"
      }
    },
    {
      "addon": "addon_b",
      "name": "ext_b",
      "extension_group": "some_group",
      "app": "localhost",
      "api": {},
      "property": null
    }
  ]
}
```

{% endcode %}

The element type of the `data` array is defined as follows.

| key             | value type | required | description                                                           |
| :-------------- | :--------: | :------: | :-------------------------------------------------------------------- |
| app             |   string   |    N     | The uri of the app this extension belongs to, default is 'localhost'. |
| extension_group |   string   |    Y     | The extension_group this extension running on.                        |
| addon           |   string   |    Y     | The addon used to create this extension.                              |
| name            |   string   |    Y     | The extension name.                                                   |
| api             |   object   |    N     | The schema definitions of property and msgs of this extension.        |
| property        |   object   |    N     | The property of this extension.                                       |

> Note that each element in the `data` array is uniquely identified by the combination of `app`, `extension_group`, and `name`.

Definition of the `api` object.

| key             | value type | required | description                              |
| :-------------- | :--------: | :------: | :--------------------------------------- |
| property        |   object   |    N     | The schema of the property.              |
| cmd_in          |   object   |    N     | The schema of all the `IN` cmd.          |
| cmd_out         |   object   |    N     | The schema of all the `OUT` cmd.         |
| data_in         |   object   |    N     | The schema of all the `IN` data.         |
| data_out        |   object   |    N     | The schema of all the `OUT` data.        |
| audio_frame_in  |   object   |    N     | The schema of all the `IN` audio_frame.  |
| audio_frame_out |   object   |    N     | The schema of all the `OUT` audio_frame. |
| video_frame_in  |   object   |    N     | The schema of all the `IN` video_frame.  |
| video_frame_out |   object   |    N     | The schema of all the `OUT` video_frame. |

> Note that the `cmd`, `data`, `audio_frame`, `video_frame` are four types of TEN msgs.

The format of the `property` is same as the schema definition. The format of `data_in` / `data_out` / `audio_frame_in` / `audio_frame_out` / `video_frame_in` / `video_frame_out` are same as follows.

| key                   | value type | required | description                             |
| :-------------------- | :--------: | :------: | :-------------------------------------- |
| name                  |   string   |    Y     | The msg name.                           |
| property              |   array    |    N     | The property belongs to this msg.       |
| property[].name       |   string   |    Y     | The property name.                      |
| property[].attributes |   object   |    Y     | The schema definition of this property. |

The format of the `cmd_in` and `cmd_out` are same, and compared with the above `data_in`, there is an additional `result` attribute.

| key                   | value type | required | description                                                                      |
| :-------------------- | :--------: | :------: | :------------------------------------------------------------------------------- |
| name                  |   string   |    Y     | The msg name.                                                                    |
| property              |   array    |    N     | The property belongs to this msg.                                                |
| property[].name       |   string   |    Y     | The property name.                                                               |
| property[].attributes |   object   |    Y     | The schema definition of this property.                                          |
| result                |   object   |    N     | The schema of the corresponding result of this cmd.                              |
| result.property       |   array    |    Y     | The property belongs to this cmd result, the format is same with the `property`. |

## Connections in a Specified Graph

Retrieve the list of connections within a specified graph.

- **Endpoint:** `/api/dev-server/v1/graphs/{graph_id}/connections`
- **Verb:** GET

You will receive a `200 OK` response. Ex:

{% code title=".json" %}

```json
{
  "status": "ok",
  "data": [
    {
      "app": "localhost",
      "extension_group": "some_group",
      "extension": "ext_a",
      "cmd": [
        {
          "name": "cmd_1",
          "dest": [
            {
              "app": "localhost",
              "extension_group": "some_group",
              "extension": "ext_b",
              "msg_conversion": {
                "type": "per_property",
                "rules": [
                  {
                    "path": "extra_data",
                    "conversion_mode": "fixed_value",
                    "value": "tool_call"
                  }
                ],
                "keep_original": true
              }
            }
          ]
        }
      ]
    }
  ]
}
```

{% endcode %}

The element type of the `data` array is defined as follows.

| key             | value type | required | description                                                                                                                   |
| :-------------- | :--------: | :------: | :---------------------------------------------------------------------------------------------------------------------------- |
| app             |   string   |    N     | Same as the `app` field in the `nodes`.                                                                                       |
| extension_group |   string   |    Y     | Same as the `extension_group` field in the `nodes`.                                                                           |
| extension       |   string   |    Y     | Same as the **`name`** field in the `nodes`.                                                                                  |
| cmd             |   array    |    N     | The msgs will be sent from this extension group by type. The possible values are `cmd`, `data`, `audio_frame`, `video_frame`. |

The element type of the `cmd` array is defined as follows.

| key                    | value type | required | description                                                           |
| :--------------------- | :--------: | :------: | :-------------------------------------------------------------------- |
| name                   |   string   |    Y     | The msg name, must be unique in each msg group.                       |
| dest                   |   array    |    Y     | The extensions this msg will be sent to.                              |
| dest[].app             |   string   |    N     | Same as the `app` field in the `nodes`.                               |
| dest[].extension_group |   string   |    Y     | Same as the `extension_group` field in the `nodes`.                   |
| dest[].extension       |   string   |    Y     | Same as the `name` field in the `nodes`.                              |
| dest[].msg_conversion  |   object   |    N     | The conversions used to transform the msg before sending to the dest. |

The definition of `msg_conversion` is as follows.

| key                    | value type | required | description                                                                |
| :--------------------- | :--------: | :------: | :------------------------------------------------------------------------- |
| type                   |   string   |    Y     | The possible value is `per_property`.                                      |
| rules                  |   array    |    Y     | The conversion rules.                                                      |
| rule[].path            |   string   |    Y     | The json path of the property this rule will be applied on.                |
| rule[].conversion_mode |   string   |    Y     | The method of rule, possible values are `fixed_value` and `from_original`. |
| rule[].value           |   string   |    N     | Required if the conversion_mode is `fixed_value`.                          |
| rule[].original_path   |   string   |    N     | Required if the conversion_mode is `from_original`.                        |
| keep_original          |  boolean   |    N     | Whether to clone the original property. Default is false.                  |

## Retrieve Compatible Messages for a Selected Extension

Select a message from an extension and retrieve all other messages from different extensions in the graph that are compatible with it.

- **Endpoint:** `/api/dev-server/v1/messages/compatible`
- **Verb:** POST

The input body is a JSON object that represents a request to find compatible pins (connections) for an output command from a specific extension within a specified graph.

{% code title=".json" %}

```json
{
  "app": "localhost",
  "graph": "default",
  "extension_group": "extension_group_1",
  "extension": "extension_1",
  "msg_type": "cmd",
  "msg_direction": "out",
  "msg_name": "test_cmd"
}
```

{% endcode %}

You will receive a `200 OK` response, with the body containing a JSON array like this:

{% code title=".json" %}

```json
[
  {
    "app": "localhost",
    "extension_group": "extension_group_1",
    "extension": "extension_2",
    "msg_type": "cmd",
    "msg_direction": "in",
    "msg_name": "test_cmd"
  },
  {
    "app": "localhost",
    "extension_group": "extension_group_1",
    "extension": "extension_3",
    "msg_type": "cmd",
    "msg_direction": "in",
    "msg_name": "test_cmd"
  }
]
```

{% endcode %}

## Update a Graph

Update the specified graph.

- **Endpoint:** `/api/dev-server/v1/graphs/{graph_id}`
- **Verb:** PUT

Input data (body):

{% code title=".json" %}

```json
{
  "auto_start": false,
  "nodes": [
    {
      "name": "extension_1",
      "addon": "extension_addon_1",
      "extension_group": "extension_group_1",
      "app": "localhost"
    },
    {
      "name": "extension_2",
      "addon": "extension_addon_2",
      "extension_group": "extension_group_1",
      "app": "localhost",
      "property": {
        "a": 1
      }
    }
  ],
  "connections": [
    {
      "app": "localhost",
      "extension_group": "extension_group_1",
      "extension": "extension_1",
      "cmd": [
        {
          "name": "hello_world",
          "dest": [
            {
              "app": "localhost",
              "extension_group": "extension_group_1",
              "extension": "extension_2"
            }
          ]
        }
      ]
    }
  ]
}
```

{% endcode %}

If successful, the client will receive a `200 OK` response; otherwise, a `40x` error code will be returned.

## Save `manifest.json`

Save the `manifest.json` file.

- **Endpoint:** `/api/dev-server/v1/manifest`
- **Verb:** PUT

If successful, the client will receive a `200 OK` response; otherwise, a `40x` error code will be returned.

## Save `property.json`

Save the `property.json` file, including predefined graphs and other content.

- **Endpoint:** `/api/dev-server/v1/property`
- **Verb:** PUT

If successful, the client will receive a `200 OK` response; otherwise, a `40x` error code will be returned.
