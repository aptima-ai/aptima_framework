# Interface

The APTIMA framework’s message system provides a mechanism for interaction at the granularity of a single message. However, in practice, a complete feature typically involves multiple messages, such as several commands, data exchanges, etc. For example, a Speech-to-Text (STT) functionality may require several commands and data messages. These fixed combinations of messages form what is known as an **interface**. If an extension supports all the messages defined by an interface, it can be used in any context that requires that interface.

Instead of requiring extensions to explicitly declare support for every message in an interface in their manifest, which would be cumbersome, the APTIMA framework introduces a concept of **aggregating** these messages. This allows extensions to declare that they either provide or utilize a specific interface, simplifying the process.

The following APIs provided by the APTIMA framework are primitive:

- `cmd_in`
- `cmd_out`
- `data_in`
- `data_out`
- `audio_frame_in`
- `audio_frame_out`
- `video_frame_in`
- `video_frame_out`

In addition to these, the framework also supports composite API mechanisms:

- `interface_in`
- `interface_out`

## Interface Syntax

The basic syntax for defining an interface includes a mandatory `name` field. This `name` serves the same purpose as the `name` field in `cmd_in` and `cmd_out`, representing the name of the interface. It is significant only within the extension’s context and must be unique within that scope.

{% code title=".json" %}

```json
{
  "api": {
    "interface_in": [
      {
        "name": "foo"
      }
    ],
    "interface_out": [
      {
        "name": "foo"
      }
    ]
  }
}
```

{% endcode %}

## Using Interfaces in Graphs

The `name` of an interface is primarily used in graphs to specify routing. In the example below, `src_extension` uses the `foo` interface provided by `dest_extension`. The `src_extension` recognizes the `foo` interface from its `interface_out` definition, while `dest_extension` recognizes the `foo` interface from its `interface_in` definition.

{% code title=".json" %}

```json
{
  "predefined_graphs": [{
    "name": "default",
    "auto_start": true,
    "nodes": [
      {
        "type": "extension_group",
        "name": "default_extension_group",
        "addon": "default_extension_group"
      },
      {
        "type": "extension",
        "name": "src_extension",
        "addon": "src_extension",
        "extension_group": "default_extension_group"
      },
      {
        "type": "extension",
        "name": "dest_extension",
        "addon": "dest_extension",
        "extension_group": "default_extension_group"
      }
    ],
    "connections": [{
      "extension": "src_extension",
      "interface": [{
        "name": "foo",
        "dest": [{
          "extension": "dest_extension"
        }]
      }]
    }]
  }]
}
```

{% endcode %}

## Meaning of `interface_in` and `interface_out`

1. **`interface_in`**

   Indicates that the extension supports the specified interface's functionality.

{% code title=".json" %}

   ```json
   {
     "api": {
       "interface_in": [
         {
           "name": "foo"
           // Interface content
         }
       ]
     }
   }
   ```

{% endcode %}

2. **`interface_out`**

   Indicates that the extension requires another extension to provide the specified interface's functionality.

{% code title=".json" %}

   ```json
   {
     "api": {
       "interface_out": [
         {
           "name": "foo"
           // Interface content
         }
       ]
     }
   }
   ```

{% endcode %}

## Interface and Message Declaration

When an extension declares an interface in its manifest, it implicitly declares all the messages defined within that interface. For instance, if an interface `foo` defines three commands, then:

- **`interface_in`**: The extension provides these three commands as part of the interface.
- **`interface_out`**: The extension requires these three commands from another extension.

In this sense, the interface mechanism serves as **syntactic sugar**, allowing a predefined set of messages to be bundled and reused across multiple extensions.

If an extension defines an interface in its manifest, it does not need to separately define the messages included in that interface. When the extension sends or receives a command like `foo`, the APTIMA runtime will look through the extension’s manifest for the relevant interface definition and use the command accordingly.

For example, if an interface defines three commands and one data message, an extension that declares this interface in `interface_in` implicitly includes these three commands in `cmd_in` and the data message in `data_in`. Similarly, if the interface is declared in `interface_out`, the commands are included in `cmd_out`, and the data message in `data_out`.

## Supporting Multiple Interfaces with the Same Message Name

In the current design, an extension cannot declare support for two interfaces with the same message name under a single API item. For example, if both `foo` and `bar` interfaces define a command named `xxx`, the following combinations are either allowed or not allowed:

- Not allowed

{% code title=".json" %}

  ```json
  {
    "api": {
      "interface_in": [
        {
          "name": "foo"
          // Interface foo content
        },
        {
          "name": "bar"
          // Interface bar content
        }
      ]
    }
  }
  ```

{% endcode %}

- Allowed

{% code title=".json" %}

  ```json
  {
    "api": {
      "interface_out": [
        {
          "name": "foo"
          // Interface foo content
        },
        {
          "name": "bar"
          // Interface bar content
        }
      ]
    }
  }
  ```

   {% endcode %}

{% tab title="Allowed" %}

{% code title=".json" %}

  ```json
  {
    "api": {
      "interface_in": [
        {
          "name": "foo"
          // Interface foo content
        }
      ],
      "interface_out": [
        {
          "name": "bar"
          // Interface bar content
        }
      ]
    }
  }
  ```

{% endcode %}
{% endtab %}

## Interface Definition Content

The definition of an `interface` is similar to the `api` field in a manifest. Below is an example of an `interface` definition:

{% code title=".json" %}

```json
{
  "cmd": [
    {
      "name": "cmd_foo",
      "property": {
        "foo": {
          "type": "int8"
        },
        "bar": {
          "type": "string"
        }
      },
      "result": {
        "property": {
          "aaa": {
            "type": "int8"
          },
          "bbb": {
            "type": "string"
          }
        }
      }
    }
  ],
  "data": [
    {
      "name": "data_foo",
      "property": {
        "foo": {
          "type": "int8"
        },
        "bar": {
          "type": "string"
        }
      }
    }
  ],
  "video_frame": [],
  "audio_frame": []
}
```

  {% endcode %}

The APTIMA framework processes this interface by integrating its definitions into the extension’s manifest under the `api` field. For instance, commands defined in `interface_in` are integrated into `cmd_in`, and those in `interface_out` are integrated into `cmd_out`.

## Specifying Interface Content

There are two ways to specify the content of an interface:

{% tabs %}
{% tab title="Directly Embed the Content" %}

Directly embed the interface definition in the manifest. Example:
{% code title=".json" %}

```json
{
  "api": {
    "interface_in": [
      {
        "name": "foo",
        "cmd": [],
        "data": [],
        "video_frame": [],
        "audio_frame": []
      }
    ]
  }
}
```

{% endcode %}
{% endtab %}

{% tab title="Reference the Content" %}

Use a reference to specify the interface definition, similar to the `$ref` syntax in JSON schema.

{% code title=".json" %}

```json
{
  "api": {
    "interface_in": [
      {
        "name": "foo",
        "$ref": "http://www.github.com/darth_vader/fight.json"
      }
    ]
  }
}
```

{% endcode %}
{% endtab %}

## Determining Interface Compatibility

Since an interface is essentially syntactic sugar, whether two interfaces can be connected depends on the compatibility of the underlying messages. When the source extension specifies an output interface `foo`, and the destination extension specifies an input interface `bar`, the APTIMA runtime checks whether the `foo` interface of the source can connect to the `bar` interface of the destination.

{% code title=".json" %}

```json
{
  "api": {
    "interface_out": "foo"
    // Interface content
  }
}
```

  {% endcode %}

{% code title=".json" %}

```json
{
  "api": {
    "interface_in": "bar"
    // Interface content
  }
}
```

  {% endcode %}

{% code title=".json" %}

```json
{
  "extension_group": {
    "addon": "default_extension_group",
    "name": "default_extension_group"
  },
  "extension": {
    "addon": "extension_A",
    "name": "extension_A"
  },
  "interface": [
    {
      "name": "foo",
      "dest": [
        {
          "extension_group": {
            "addon": "default_extension_group",
            "name": "default_extension_group"
          },
          "extension": {
            "addon": "dest_extension",
            "name": "dest_extension"
          }
        }
      ]
    }
  ]
}
```

  {% endcode %}

The APTIMA framework will look for the definition of `foo` in the `interface_out` section of the source extension's manifest. It then checks each message defined in this interface against the destination extension’s manifest, both in its message definitions and in any interfaces it declares in `interface_in`. If any message cannot be matched according to the APTIMA framework’s schema-checking rules, the graph configuration will be considered invalid.
