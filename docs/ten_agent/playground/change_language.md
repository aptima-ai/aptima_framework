# Change language of your agent

When we talk about the language of the agent, we are referring to following aspects,

- The language in which agent understands the user's audio input. i.e. STT language
- The language in which agent responds to the user. i.e. TTS language

You will need to configure different properties of above extensions depending on your requirements.

## Change STT language for STT + LLM + TTS agents

To change the language in which agent understands the user's audio input, you will need to change the properties of the STT extension. You can do this by following the steps below,

1. Open the playground at [localhost:3000](http://localhost:3000) to configure your agent.
2. Select "voice_assistant" graph type.
3. Click on the Button to the right of the graph selection to open the properties configuration.
4. Choose the "STT" extension from the dropdown list.
5. Change the language property to the desired language.
6. Click on `Save Change` to apply the language to the STT extension.

> Note: The language property of the STT extension differs based on the STT service provider. Please refer to the documentation of the STT service provider for the list of supported options.

## Change STT language for STT + LLM + TTS agents (Use RTC Integrated Azure STT)

To change the language in which agent understands the user's audio input, you will need to change the properties of the RTC extension, as STT extension is integrated within it. You can do this by following the steps below,

1. Open the playground at [localhost:3000](http://localhost:3000) to configure your agent.
2. Select "voice_assistant_stt_integrated" graph type.
3. Click on the Button to the right of the graph selection to open the properties configuration.
4. Choose the "RTC" extension from the dropdown list.
5. Change the language property to the desired language. For Azure, 'en-US' stands for English. You can find more language options in the Azure documentation.
6. Click on `Save Change` to apply the language to the RTC extension.

## Change language that agent speaks

> Note: The language TTS support usually are determined by the voice of the TTS. Some TTS support multilingual voices, while some support only a single language.

To change the language in which agent responds to the user, you will need to change the properties of the TTS extension. You can do this by following the steps below,

1. Open the playground at [localhost:3000](http://localhost:3000) to configure your agent.
2. Select "voice_assistant" graph type.
3. Click on the Button to the right of the graph selection to open the properties configuration.
4. Choose the "TTS" extension from the dropdown list.
5. Change the voice property to the desired voice of the language.
6. Click on `Save Change` to apply the language to the TTS extension.

> Note: The voice property of the TTS extension differs based on the TTS service provider. Please refer to the documentation of the TTS service provider for the list of supported options.
