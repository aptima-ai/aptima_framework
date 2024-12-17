# Run Voice Assistant with Voice to Voice Realtime API

This guide will help you to run the Voice Assistant with Voice to Voice Realtime API in the TEN-Agent Playground.

## Prerequisites

- Make sure you have the TEN-Agent playground running. If not, follow the [Run Playground](https://doc.theten.ai/ten-agent/quickstart) guide to start the playground.
- You will need following information prepared:
  - Realtime API Key
- RTC info, currently only Agora RTC is supported. You can register your account at [Agora](https://www.agora.io/). We assume you have your App ID and App Certificate ready when you configure your `.env` file.

## Steps

1. Open the playground at [localhost:3000](http://localhost:3000) to configure your agent.
2. Select the graph type `voice_assistant_realtime`.
3. Click on `Module Picker` to open the module selection.
4. Select your preferred V2V module from the dropdown list.
5. Click on `Save Change` to apply the module to the graph.
6. Click on the Button to the right of the graph selection to open the property configuration. You will see a list of properties that can be configured for the selected V2V module.
7. Configure the `Realtime API Key` property with the information you prepared.
8. Click on `Save Change` to apply the property to the V2V module.
9. If you see the success toast, the property is successfully applied to the V2V module.
10. You are all set! Now you can start speaking to the Voice Assistant by clicking on the `Connect` button. Note you will need to wait a few seconds for agent to initialzie itself.

## Bind Weather Tool to your V2V

You can bind weather tool to your V2V module in the TEN-Agent Playground.

1. When you have your agent running. Open Module Picker.
2. Click on the button to the right of the V2V module to open the tool selection.
3. Select `Weather Tool` from the popover list.
4. Click on `Save Change` to apply the tool to the V2V module.
5. If you see the success toast, the tool is successfully applied to the V2V module.
6. You are all set! Now you can ask the agent about the weather by speaking to it.
