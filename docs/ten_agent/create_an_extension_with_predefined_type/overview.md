# Overview

## Extension types

When developing Extensions, we often notice that implementations for Extensions of the same category share similarities. For example, the Extensions for Gemini and OpenAI have similar implementation logic, but they also differ in certain details. To improve development efficiency, these similar Extension implementations can be abstracted into a generic Extension type. During actual development, you only need to inherit from this type and implement a few specific methods.

Currently, APTIMA Agent supports the following Extension types:

- `AsyncLLMBaseExtension`: Designed for implementing large language model Extensions, such as those similar to OpenAI.
- `AsyncLLMToolBaseExtension`: Used to implement tool Extensions for large language models. These are Extensions that provide tool capabilities based on Function Call mechanisms.

This abstraction helps standardize development while reducing repetitive work.

You can execute the following command in the APTIMA project to install the abstract base class library:

```bash
tman install system ten_ai_base@0.1.0
```

## Extension behavior

### LLM Extension and LLMTool Extension

Any LLMTool Extension can be connected with an LLM Extension. When the LLMTool is started, it will automatically connect to the LLM Extension.

When the LLM Extension detects a Function Call, it will pass the Function Call to the LLMTool Extension for processing. Once the LLMTool Extension has completed the processing, it will return the result to the LLM Extension.
