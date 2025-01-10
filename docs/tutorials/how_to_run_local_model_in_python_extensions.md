---
layout:
  title:
    visible: true
  description:
    visible: false
  tableOfContents:
    visible: true
  outline:
    visible: true
  pagination:
    visible: true
---

# How to Run a Local AI Model in Python Extensions

In the APTIMA framework, extensions can utilize third-party AI services or run AI models locally to improve performance and reduce costs. This tutorial explains how to run a local AI model in a Python extension and how to interact with it within the extension.

## Step 1: Check Hardware Requirements

Before running an AI model locally, ensure that your hardware meets the necessary requirements. Key components to verify include:

- **CPU/GPU**: Check if the model requires specific processing power.
- **Memory**: Ensure sufficient memory to load and run the model.

Verify that your system can support the model’s demands to ensure smooth operation.

## Step 2: Install Necessary Software and Dependencies

Once your hardware is ready, install the required software and dependencies. Follow these steps:

1. **Operating System**: Ensure compatibility with your model. Most AI frameworks support Windows, macOS, and Linux, though specific versions may be required.
2. **Python Version**: Ensure compatibility with the APTIMA Python runtime and the model.
3. **Required Libraries**: Install necessary libraries such as:

   - TensorFlow
   - PyTorch
   - Numpy
   - vllm

   You can list the required dependencies in a `requirements.txt` file for easy installation.

4. **Download the Model**: Obtain the local version of the AI model you plan to run.

## Step 3: Implement Your Python Extension

Below is an example of how to implement a basic text generation feature using the `vllm` inference engine in a Python extension.

First, initialize the local model within the extension:

```python
from aptima import (
    Extension,
    TenEnv,
    Cmd,
    CmdResult,
)
from vllm import LLM

class TextGenerationExtension(Extension):
    def on_init(self, ten_env: TenEnv) -> None:
        self.llm = LLM(model="<model_path>")
        ten_env.on_init_done()
```

Next, implement the `on_cmd` method to handle text generation based on the provided input:

```python
    def on_cmd(self, ten_env: TenEnv, cmd: Cmd) -> None:
        prompt = cmd.get_property_string("prompt")

        outputs = self.llm.generate(prompt)
        generated_text = outputs[0].outputs[0].text

        cmd_result = CmdResult.create(StatusCode.OK)
        cmd_result.set_property_string("result", generated_text)
        ten_env.return_result(cmd_result, cmd)
```

In this code, the `on_cmd` method retrieves the `prompt`, generates text using the model, and returns the generated text as the command result.

You can adapt this approach to implement other functionalities such as image recognition or speech-to-text by processing the relevant input types.

## Step 4: Unload the Model

It’s important to unload the model during extension cleanup to free resources:

```python
import gc
import torch

class TextGenerationExtension(Extension):
    ...

    def on_deinit(self, ten_env: TenEnv) -> None:
        del self.llm
        gc.collect()
        torch.cuda.empty_cache()
        torch.distributed.destroy_process_group()
        print("Successfully deleted the LLM pipeline and freed GPU memory!")
        ten_env.on_deinit_done()
```

This ensures efficient memory management, especially when working with GPU resources.

## Summary

Running a local model in a APTIMA Python extension is similar to native Python development. By loading and unloading the model in the appropriate extension lifecycle methods, you can easily integrate local AI models and interact with them effectively.
