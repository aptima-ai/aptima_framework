# Architecture Flow

The whole system contains multiple components that work together to provide a seamless experience. The main components are:

- **TEN Agent App**: The main application that orchestrates the extensions and manages the data flow between them. It runs as a background process. Depending on the graph configuration, the agent starts the required extensions and handles the data flow between them.
- **Front-end UI**: A web-based interface to interact with the agent. It allows users to configure the agent, start/stop the agent, and talk to the agent.
- **Web Server**: A simple Golang web server that serves the HTTP requests. It is responsible for handling the incoming requests and starting/stopping agent processes. It also passes parameter like `graph_name` to determine which graph to use.

The flow of the system is as follows:

![Architecture Flow](https://github.com/TEN-framework/docs/blob/main/assets/png/architecture_flow.png?raw=true)
