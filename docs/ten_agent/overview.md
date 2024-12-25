# Overview

TEN Agent is a conversational AI agent powered by the TEN, integrating Gemini 2.0 Live, OpenAI Realtime, RTC, and more. It delivers real-time capabilities to see, hear, and speak, while being fully compatible with popular workflow platforms like Dify and Coze.

## Links

- [TEN Agent](https://github.com/TEN-framework/ten_agent)
- [TEN Framework](https://github.com/TEN-framework/ten_framework)

## Architecture

The TEN Agent project is organized into the following major components, offering clarity and extensibility for developers:

1.	**Agents**: Contains the core logic, binaries, and examples for building and running AI agents. Within the Agents folder, there is a subfolder called `ten_packages,` which houses a variety of ready-to-use extensions. By leveraging these extensions, developers can build and customize powerful agents tailored to specific tasks or workflows.

2.	**Dev Server**: Backend services, orchestrating agents and handling extensions.
3.	**Web Server**: Runs on port 8080 and serves the frontend interface. The web server handles HTTP requests and delivers assets.
4.	**Extensions**: Modular integrations for LLMs, TTS/STT, and external APIs, enabling easy customization.
5.	**Playground**: An interactive environment for testing, configuring, and fine-tuning agents.
6.	**Demo**: A deployment-ready setup to showcase real-world applications of TEN Agent.

## Docker Containers

There are two Docker containers in TEN Agent:

- `ten_agent_dev`: The main development container that powers TEN Agent. It contains the core runtime environment, development tools, and dependencies needed to build and run agents. This container lets you execute commands like `task use` to build agents and `task run` to start the web server.

- `ten_agent_playground`: Port 3000, a dedicated container for the web frontend interface. It serves the compiled frontend assets and provides an interactive environment where users can configure modules, select extensions, and test their agents. The playground UI allows you to visually select graph types (like Voice Agent or Realtime Agent), choose modules, and configure API settings.

- `ten_agent_demo`: Port 3002, a deployment-focused container that provides a production-ready sample setup. It demonstrates how users can deploy their configured agents in real-world scenarios, with all necessary components packaged together for easy deployment.


## Agents

The Agents folder is the heart of the project, housing:

-	Core binaries and examples that define agent behaviors.
-	Scripts and outputs that enable flexible configurations for various AI use cases.
-	Tools for developers to create, modify, and enhance AI agents.

With its structured design, the Agents folder allows you to build agents tailored to specific applications, whether it’s voice assistants, chatbots, or task automation.

## Demo

The Demo folder provides a deployment-ready environment for showcasing TEN Agent in action. It includes:
-	Example configurations for running agents in production.
-	Prebuilt agents and workflows to highlight the framework’s capabilities.
-	Tools for demonstrating real-world applications to users, clients, or collaborators.

## Playground

Once the playground is up and running, users can leverage the module picker to:
-	Select and configure extensions from a range of prebuilt modules.
-	Experiment with different AI models, TTS/STT systems, and real-time communication tools.
-	Test agent behaviors in a safe, interactive environment.

The playground serves as a hub for innovation, empowering developers to explore and fine-tune their AI systems effortlessly.