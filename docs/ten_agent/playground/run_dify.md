# How to make Dify Chat Bot Speak

In this tutorial, we will show you how to make Dify Bot speak in TEN-Agent playground.

## Prerequisites

- Make sure you have the TEN-Agent playground running. If not, follow the [Run Playground](https://doc.theten.ai/ten-agent/getting_started) guide to start the playground.
- You will need following information prepared:
  - Dify info:
    - Dify API Key
  - STT info, any supported STT can be used. [Deepgram](https://deepgram.com/) is relatively easy to register and get started with.
  - TTS info, any supported TTS can be used. [Fish.Audio](https://fish.audio/) is relatively easy to register and get started with.
  - RTC info, currently only Agora RTC is supported. You can register your account at [Agora](https://www.agora.io/). We assume you have your App ID and App Certificate ready when you configure your `.env` file.

> You can use any Agent / Chat Assistant defined in Dify platform. Each Agent / Chat Assistant has its own API Key.

## Steps

1. Open the playground at [localhost:3000](http://localhost:3000) to configure your agent.
2. Select the graph type `voice_assistant`.
3. Click on `Module Picker` to open the module selection.
4. If you preferred STT/TTS module is not by default selected, you can select the module from the dropdown list. Note you will need to configure the module with the correct information like API key, etc.
5. From `LLM` module, click on the dropdown and select `Dify Chat Bot`.
6. Click on `Save Change` to apply the module to the graph.
7. Click on the Button to the right of the graph selection to open the property configuration. You will see a list of properties that can be configured for the `Dify Chat Bot` module.
8. Configure the `Dify API Key` property with the information you prepared.
9. Click on `Save Change` to apply the property to the `Dify Chat Bot` module.
10. If you see the success toast, the property is successfully applied to the `Dify Chat Bot` module.
11. You are all set! Now you can start speaking to Dify Bot by clicking on the `Connect` button. Note you will need to wait a few seconds for agent to initialzie itself.

## Using Azure STT

Azure STT is integrated within RTC extension module. That's why if you want to use Azure STT, you will need to select `voice_assistant_integrated_stt` graph type.

## Troubleshooting

If you encounter any issues, please check the following:

- Make sure you have the correct API Key for Dify Chat Bot.
- Make sure in Dify your Chat Assistant has a valid model key configured.
- Make sure you have the correct STT and TTS modules selected and configured.
- Make sure you have the correct graph type selected.
