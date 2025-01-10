# Run APTIMA-Agent Demo

This guide will help you to run the APTIMA-Agent demo. The demo is a web-based interface that allows you to interact with the APTIMA-Agent. The orchestration of demo site is fixed and cannot be changed.

## Run demo from pre-built Docker image

The easiest way to run the demo is to use the pre-built Docker image. The image contains the latest version of the APTIMA-Agent and the demo. The project docker compose file already contains the pre-built image, so you can start by following the steps in [getting started guide](https://doc.theten.ai/aptima-agent/getting_started).

After you successfully setup, you will need to run an additional command to switch graph folder to demo folder,

```bash
task use AGENT=agents/examples/demo
```

Once finished, you can access the demo by opening the browser and navigating to `http://localhost:3002`.

## Run demo from source code

If you want to run the demo from the source code, start a new terminal, change to the folder where you cloned APTIMA-Agent project, follow below steps:

```bash
# Change to the demo directory
cd demo

# Install dependencies
pnpm install

# Start the demo
pnpm dev
```

Once the demo is started, you can access it by opening the browser and navigating to `http://localhost:3001`.

> **Note:** The demo depends on the golang web server (located in `server` directory). The web server will start when you run `task run` command in the container after you follow the steps in the [getting started guide](https://doc.theten.ai/aptima-agent/getting_started).
