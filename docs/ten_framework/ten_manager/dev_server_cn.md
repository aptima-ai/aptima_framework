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

You will receive a `200 OK` response, with the body containing a JSON array.

## Connections in a Specified Graph

Retrieve the list of connections within a specified graph.

- **Endpoint:** `/api/dev-server/v1/graphs/{graph_id}/connections`
- **Verb:** GET

You will receive a `200 OK` response, with the body containing a JSON array like this:

{% code title=".json" %}

```json
[
  {
    "app": "localhost",
    "extension": "simple_http_server_cpp",
    "extension_group": "default_extension_group",
    "cmd": [
      {
        "name": "hello_world",
        "dest": [
          {
            "app": "localhost",
            "extension": "default_extension_cpp",
            "extension_group": "default_extension_group"
          }
        ]
      }
    ]
  }
]
```

{% endcode %}

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
