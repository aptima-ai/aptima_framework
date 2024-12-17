# Run TEN-Agent Playground

This guide will help you to run the TEN-Agent Playground. The playground is a web-based interface that allows you to interact with the TEN-Agent. You can replace modules, change configurations, and test the agent's behavior in a controlled environment.

## Run playground from pre-built Docker image

The easiest way to run the playground is to use the pre-built Docker image. The image contains the latest version of the TEN-Agent and the playground. The project docker compose file already contains the pre-built image, so you can start by following the steps in [getting started guide](https://doc.theten.ai/ten-agent/getting_started).

Once finished, you can access the playground by opening the browser and navigating to `http://localhost:3000`.

## Run playground from source code

If you want to run the playground from the source code, start a new terminal, change to the folder where you cloned TEN-Agent project, follow below steps:

```bash
# Change to the playground directory
cd playground

# Install dependencies
pnpm install

# Start the playground
pnpm dev
```

Once the playground is started, you can access it by opening the browser and navigating to `http://localhost:3001`.

> **Note:** The playground depends on the golang web server (located in `server` directory) and ten-dev-server (provided by ten framework cli). The ten-dev-server is by default provided when you start with `docker compose up -d` command. The web server will start when you run `task run` command in the container after you follow the steps in the [getting started guide](https://doc.theten.ai/ten-agent/getting_started).
