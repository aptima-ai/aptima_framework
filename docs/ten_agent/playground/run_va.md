# Run Voice Assistant with Large Language Model

This guide will help you to run the Voice Assistant with Large Language Model in the TEN-Agent Playground.

## Prerequisites

- Make sure you have the TEN-Agent playground running. If not, follow the [Run Playground](https://doc.theten.ai/ten-agent/quickstart) guide to start the playground.
- You will need following information prepared:
  - STT info, any supported STT can be used. [Deepgram](https://deepgram.com/) is relatively easy to register and get started with.
  - TTS info, any supported TTS can be used. [Fish.Audio](https://fish.audio/) is relatively easy to register and get started with.
  - LLM info, any supported LLM can be used. It's recommended to use [OpenAI](https://openai.com)
  - RTC info, currently only Agora RTC is supported. You can register your account at [Agora](https://www.agora.io/). We assume you have your App ID and App Certificate ready when you configure your `.env` file.

## Steps

1. Open the playground at [localhost:3000](http://localhost:3000) to configure your agent.
2. Select the graph type `voice_assistant`.
3. Click on `Module Picker` to open the module selection.
4. If you preferred STT/TTS module is not by default selected, you can select the module from the dropdown list. Note you will need to configure the module with the correct information like API key, etc.
5. From `LLM` module, click on the dropdown and select your preferred Large Language Model.
6. Click on `Save Change` to apply the module to the graph.
7. Click on the Button to the right of the graph selection to open the property configuration. You will see a list of properties that can be configured for the selected Large Language Model.
8. Configure the properties with the information you prepared.
9. Click on `Save Change` to apply the properties to the Large Language Model.
10. If you see the success toast, the properties are successfully applied to the Large Language Model.
11. You are all set! Now you can start speaking to the Voice Assistant by clicking on the `Connect` button. Note you will need to wait a few seconds for agent to initialzie itself.

## Using Azure STT

Azure STT is integrated within RTC extension module. That's why if you want to use Azure STT, you will need to select `voice_assistant_integrated_stt` graph type.

## Bind Weather Tool to your LLM

You can bind weather tool to your LLM module in the TEN-Agent Playground.
It's recommended to use OpenAI LLM below.

1. When you have your agent running. Open Module Picker.
2. Click on the button to the right of the LLM module to open the tool selection.
3. Select `Weather Tool` from the popover list.
4. Click on `Save Change` to apply the tool to the LLM module.
5. If you see the success toast, the tool is successfully applied to the LLM module.
6. You are all set! Now you can ask the agent about the weather by speaking to it.
