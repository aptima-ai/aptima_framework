# TEN Agent Project Overview

The TEN Agent project is built on top of the TEN Framework. For basic concepts of a TEN Framework project, please refer to the [TEN Framework Project Overview](https://doc.theten.ai/ten-framework/concept_overview).

Below is the folder structure of TEN Agent project,

![Project structure](https://raw.githubusercontent.com/TEN-framework/docs/refs/heads/main/assets/png/folder_structure.png)

It contains the following important folders and files:

- `property.json`: This file contains the orchestration of extensions. It is the main runtime configuration file.
- `ten_packages/extension`: This folder contains the extension modules. Each extension module is a separate Python/Golang/C++ package.
- `server`: This folder contains the web server code. It is responsible for handling the incoming requests and start/stop of agent processes.
- `playground`: This folder contains the UI code for the playground. It is a web-based interface to interact with the agent.
