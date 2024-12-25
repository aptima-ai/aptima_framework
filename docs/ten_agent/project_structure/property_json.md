
# Get to know property.json

This file contains the orchestration of extensions. It is the main runtime configuration file.

![Graph at a glance](https://github.com/TEN-framework/docs/blob/main/assets/png/graph_at_a_glance.png?raw=true)

The `property.json` file contains the following orchestration info,

## Graphs

The `predefined_graphs` property contains a list of available graphs. Each graph defines how agent should behave in a specific scenario. Each graph has a `name` property which is used to identify a graph.

Each graph contains `nodes` and `connections` properties.

## Nodes

The `nodes` section contains the list of extensions that are part of the graph. Each node has a `name` property which is used to identify a node, and a `addon` property which is used to specify the extension module it will use. You can have multiple nodes of the same extension type in a graph. e.g. you can have multiple `chatgpt_openai_python` extensions with same `addon` property, while each node will have a different `name` property. It's working like multiple instances of the same extension.

#### Node Property

The `property` section of a node contains the configuration of the extension. It is extension specific and available properties are defined in the `manifest.json` file under extension folder. You can use define runtime properties of an extension to customize its behavior. Below is an example of how to configure `chatgpt_openai_python` extension,

```json
{
  "name": "chatgpt_openai_python",
  "addon": "chatgpt_openai_python",
  "property": {
    "api_key": "${env:OPENAI_API_KEY|}",
    "model": "gpt-3.5-turbo",
    "temperature": 0.5,
    "max_tokens": 100,
    "prompt": "You are a helpful assistant"
  }
}
```

![Property JSON nodes](https://github.com/TEN-framework/docs/blob/main/assets/png/property_json_nodes.png?raw=true)

#### Read environment variables

It's quite common that an extension will require `api_key` to work. If you don't want to hardcode the `api_key` in the `property.json` file, you can use environment variables. You can use `${env:<env_var_name>|<default_value>}` syntax to read environment variables. Below is an example of how to read `OPENAI_API_KEY` environment variable,

```json
{
  "name": "chatgpt_openai_python",
  "addon": "chatgpt_openai_python",
  "property": {
    "api_key": "${env:OPENAI_API_KEY|}"
  }
}
```

## Connections

The `connections` section contains the list of connections between nodes. Each connection specifies the source and destination nodes respectively. Every connection has an `extension_group` and `extension` property which is used to specify the source node, and it defines a set of multimodal data protocols(audio_frame, video_frame, data, cmd) supported by TEN Framwork. For each data protocols, it has a list of destination definitions, and each destination definition has a `name` property which is the key of the property data, and `dest` property which defines a list of destination nodes. Below is an example of how to connect two nodes,

```json
{
  "extension_group": "default",
  "extension": "agora_rtc",
  "audio_frame": [
    {
      "name": "pcm_frame",
      "dest": [{
        "extension_group": "default",
        "extension": "deepgram_asr"
      }]
    }
  ]
}
```

![Property JSON Connections](https://github.com/TEN-framework/docs/blob/main/assets/png/property_json_connections.png?raw=true)

In the above example, we are connecting `agora_rtc` extension to `deepgram_asr` extension. The `agora_rtc` extension is sending `pcm_frame` data to `deepgram_asr` extension.