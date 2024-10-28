# How does interrupt work in TEN-Agent

## Overview

The interrupt mechanism in
[TEN-Agent](https://github.com/TEN-framework/TEN-Agent) consists of two main
parts: **Interrupt Detection** and **Interrupt Response**. This document details
both parts and explains how the interrupt command propagates through the AI
agent graph.

## Part 1: Interrupt Detection

### 1. Current Interrupt Detection Implementation

The current
([interrupt_detector_python](https://github.com/TEN-framework/TEN-Agent/tree/main/agents/ten_packages/extension/interrupt_detector_python))
extension implements a text-based interrupt detection mechanism:

```python
def on_data(self, ten: TenEnv, data: Data) -> None:
    text = data.get_property_string(TEXT_DATA_TEXT_FIELD)
    final = data.get_property_bool(TEXT_DATA_FINAL_FIELD)

    # Trigger interrupt when text is final or reaches threshold length
    if final or len(text) >= 2:
        self.send_flush_cmd(ten)
```

The interrupt detector triggers in the following cases:

1. When receiving final text (`is_final = true`)
2. When text length reaches a threshold (≥ 2 characters)

### 2. Customize Interrupt Detection

To implement your own interrupt detection logic, you can refer to the
implementation of
[interrupt_detector_python](https://github.com/TEN-framework/TEN-Agent/tree/main/agents/ten_packages/extension/interrupt_detector_python)
as an example and customize the interrupt conditions based on your specific
needs.

## Part 2: Interrupt Response

### Chain Processing in AI Agent Graph

In a typical AI agent graph, the interrupt command (`flush`) follows a chain
processing pattern:

```text
Interrupt Detector
       ↓
    LLM/ChatGPT
       ↓
      TTS
       ↓
   agora_rtc
```

Each extension in the chain follows two key steps when receiving a `flush`
command:

1. Clean up its own resources and internal state
2. Forward the `flush` command to downstream extensions

This ensures that:

- Extensions are cleaned up in the correct order
- No residual data flows through the system
- Each extension returns to a clean state before the next operation

## Conclusion

[TEN-Agent](https://github.com/TEN-framework/TEN-Agent)'s interrupt mechanism
uses a chain processing pattern to ensure orderly cleanup of all extensions in
the AI agent graph. When an interrupt occurs, each extension first cleans up its
own state and then forwards the `flush` command to downstream extensions,
ensuring a clean system state for subsequent operations.
