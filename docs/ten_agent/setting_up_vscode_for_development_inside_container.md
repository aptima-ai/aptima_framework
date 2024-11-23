# Setting Up VSCode for Development Inside Container

When developing with TEN, it's generally recommended to perform compilation and development within a container. However, if you're using VSCode outside the container, you may encounter issues where symbols cannot be resolved. This is because some environment dependencies are installed within the container, and VSCode can't recognize the container's environment, leading to unresolved header files.

To solve this, you can mount VSCode within the container so that it recognizes the container’s environment and resolves the header files accordingly. This guide will walk you through using VSCode's Dev Containers and Docker extensions to achieve this.

## Step 1: Install the Docker Extension

First, install the [Docker extension](https://marketplace.visualstudio.com/items?itemName=ms-azuretools.vscode-docker) in VSCode. This extension allows you to manage Docker containers directly within VSCode.

## Step 2: Install the Dev Containers Extension

Next, install the [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers). This extension enables VSCode to connect to Docker containers for development.

## Step 3: Start the Development Environment Using Docker Compose

This step is similar to the process outlined in the [Quick Start](./quickstart.md) guide. However, instead of running:

{% code title=">_ Terminal" %}
```shell
docker compose up
```
{% endcode %}

Using the `docker compose up -d` command, start the container in detached mode:

{% code title=">_ Terminal" %}
```shell
docker compose up -d
```
{% endcode %}

After executing this command, the container should start. Open VSCode, switch to the Docker extension, and you should see the running container.

![Docker Containers](../assets/png/docker_containers.png)

## Step 4: Connect to the Container

In the Docker extension within VSCode, find the `astra_agents_dev` container in the list and click `Attach Visual Studio Code` to connect to the container. VSCode will then open a new window that is connected to the container, where you can proceed with development.

In the Dev Container environment connected to the container, your local extensions and settings will not be applied, as this environment is within the container. Therefore, you will need to install extensions and configure settings inside the container. To install extensions within the container, open the newly launched VSCode window, click on `Extensions` in the left sidebar, search for the required extension, and follow the prompts to install it inside the container.

## Step 5: Setup breakpoint for debugging

Setting breakpoints in the code is a common practice when debugging. To set a breakpoint in the code, click on the left margin of the line number where you want to set the breakpoint. A red dot will appear, indicating that a breakpoint has been set.

![Setting Breakpoint](https://raw.githubusercontent.com/TEN-framework/docs/refs/heads/main/assets/png/setting_breakpoint.png)

if you cannot set the breakpoint, it usually means you have not installed the language extension in the container. You can install the language extension by clicking on the `Extensions` icon in the left sidebar, searching for the required extension, and following the prompts to install it.

Once you have set the breakpoint, you can start debugging by clicking on the `Run and Debug` icon in the left sidebar, selecting the `debug python` configuration, and clicking on the green play button to start debugging.

![Debug Configuration](https://github.com/TEN-framework/docs/blob/main/assets/png/debug_config.png?raw=true)

In this way, VSCode is directly starting the agent application, which means Golang web server is not paticipating in the run. Therefore, you will need to pay attention to below points:

### Which graph is being used in this mode?

Web server will help you manipulate `property.json` when starting agent to help you select the graph you want to use. However, in this mode, you will need to manually modify `property.json` to select the graph you want to use. Ten will by default select **the first graph with `auto_start` property set to true** to start.

### RTC properties

RTC feature is mandatory for your client to talk to agent server. Web server will help you generate RTC tokens and channels and update these into `property.json` for you. However, in this mode, you will need to manually modify `property.json` to update RTC tokens and channels yourself. The easiest way is to run with playground/demo first, and a temp property file will be generated, you can copy the RTC tokens and channels from there.

You shall find the temp property file path in the ping request logs like below,

```shell
2024/11/23 08:39:19 INFO handlerPing start channelName=agora_74np6e requestId=218f8e36-f4d0-4c83-a6e8-d3b4dec2a187 service=HTTP_SERVER
2024/11/23 08:39:19 INFO handlerPing end worker="&{ChannelName:agora_74np6e HttpServerPort:10002 LogFile:/tmp/astra/app-agora_74np6e-20241123_083855_000.log Log2Stdout:true PropertyJsonFile:/tmp/astra/property-agora_74np6e-20241123_083855_000.json Pid:5330 QuitTimeoutSeconds:60 CreateTs:1732351135 UpdateTs:1732351159}" requestId=218f8e36-f4d0-4c83-a6e8-d3b4dec2a187 service=HTTP_SERVER
```

The temp property file path is shown as `PropertyJsonFile:/tmp/astra/property-agora_74np6e-20241123_083855_000.json`.
