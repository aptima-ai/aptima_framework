# Overview

TEN Agent, built on the TEN framework, is an open-source AI agent project. It can speak, see and access to a knowledge base.

It also serves as a boilerplate for using the TEN framework, offering a great way to fully understand how to use the framework by diving into TEN Agent.

## Architecture

The TEN Agent consists of two main parts:

1. **Frontend**: **Playground** (Port 3000)
   - Web interface with API Router and UI
   - Manages communication and graph control

2. **Backend Services**
   - DevServer (Port 49463): Handles graph operations
   - WebServer (Port 8080): Processes commands
   - AgentApp: Core component with extensions and config

![Components Diagram](https://github.com/TEN-framework/docs/blob/main/assets/jpg/diagram.jpg?raw=true)

## Docker Containers

There are two Docker containers in TEN Agent:

- `astra_agents_dev`: This is the workspace for development. The code repository will be mirrored into the /app folder in the container, and it provides the build environment. The backend service will also run in this container.
- `astra_playground`: This is a separate service for the web frontend. The compiled frontend files will be mirrored here.

## Agents

The agents directory contains the core runtime of TEN, along with the graphs defined by users and some miscellaneous items like build scripts.

## Manifest

The application’s metadata and required extensions are specified here. Please note that `ten_runtime_go`, `py_init_extension_cpp`, and `agora_rtc` must be included. The required items will be stored in the `/ten_packages` directory.

## Property

All graph information is stored in property.json. We recommend using the Graph Designer to create and edit the graphs instead of directly modifying `property.json`. Each graph consists of a list of nodes and connections:

- In each node section, specify which extension will be used in the node, along with all required environment variables.
- In each connection section, specify how the data is passed. Data should flow from an extension to one or more destination extensions. The data format must be one of the four formats defined by TEN: Command, Data, Video Frame, or Audio Frame.

For details, see the API reference on interfaces in graphs.

## Bin

The build script will compile the graphs into binaries stored in the `bin` folder. The binaries can be called by other services, such as the server. Note that the binary must be restarted for changes in `property.json` to take effect.

## Server

The server folder includes a lightweight HTTP server and a module for running agent binaries. Below are the HTTP APIs for use. They can also be integrated with other frontend applications, such as mobile apps.

## Start

Starts an agent with the given graph and overridden properties. The started agent will join the specified channel and subscribe to the uid used by your browser/device’s RTC.

| Parameter | Description |
|-----------|-------------|
| user_uid | The uid used by your browser/device's RTC, needed by the agent to subscribe to your audio. |
| timeout | Specifies how long the agent will remain active without receiving pings. If set to -1, the agent will not terminate due to inactivity. The default is 60 seconds, but this can be adjusted with the WORKER_QUIT_TIMEOUT_SECONDS variable in your .env file. |
| request_id | A UUID for tracing requests. |
| properties | Additional properties to override in property.json (these overrides won't affect the original property.json, only the agent instance). |
| graph_name | The graph to be used when starting the agent, found in property.json. |
| channel_name | Must match the one your browser/device joins; the agent needs to be in the same channel to communicate. |
| bot_uid | (Optional) The uid used by the bot to join RTC. |

Example:

{% code title=">_ Terminal" %}
```bash
curl 'http://localhost:8080/start' \
-H 'Content-Type: application/json' \
--data-raw '{
  "request_id": "c1912182-924c-4d15-a8bb-85063343077c",
  "channel_name": "test",
  "user_uid": 176573,
  "graph_name": "camera.va.openai.azure",
  "properties": {
    "openai_chatgpt": {
      "model": "gpt-4o"
    }
  }
}'
```
{% endcode %}

## Stop

Stops the agent that was previously started.

| Parameter | Description |
|-----------|-------------|
| request_id | A UUID for tracing requests. |
| channel_name | The channel name used to start the agent. |

Example:

{% code title=">_ Terminal" %}
```bash
curl 'http://localhost:8080/stop' \
-H 'Content-Type: application/json' \
--data-raw '{
  "request_id": "c1912182-924c-4d15-a8bb-85063343077c",
  "channel_name": "test"
}'
```
{% endcode %}

## Ping

Sends a ping to the server to indicate the connection is still alive. This is unnecessary if you specify timeout: -1 when starting the agent. Otherwise, the agent will quit if it doesn’t receive a ping after the specified timeout.

Example:

{% code title=">_ Terminal" %}
```bash
curl 'http://localhost:8080/ping' \
-H 'Content-Type: application/json' \
--data-raw '{
  "request_id": "c1912182-924c-4d15-a8bb-85063343077c",
  "channel_name": "test"
}'
```
{% endcode %}

## Playground

Playground is the UI of TEN Agent. It is built with NextJS. You can preview it online at <https://agent.theten.ai/>.

The code to handle audio input/output and transcribed text is in src/manager/rtc/rtc.ts. The code captures user audio and transmits it to the agent server while the agent’s audio is sent back to the web app.

Example of joining a channel:

{% code title=">_ Terminal" %}
```bash
async join({ channel, userId }: { channel: string; userId: number }) {
  if (!this._joined) {
    const res = await apiGenAgoraData({ channel, userId });
    const { code, data } = res;
    if (code !== 0) {
      throw new Error("Failed to get token");
    }
    const { appId, token } = data;
    await this.client?.join(appId, channel, token, userId);
    this._joined = true;
  }
}
```
{% endcode %}

The text message is transmitted from the agent server to the web app via the Stream Message callback:

{% code title=">_ Terminal" %}
```bash
const onStreamMessage = (message: any) => {
  const { text } = message;
  if (text) {
    this.setState({ text });
  }
};
```
{% endcode %}
