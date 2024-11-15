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

# Create a LLM Extension

## Creating AsyncLLMBaseExtension by using tman

Run the following command,

```bash
tman install extension default_async_llm_extension_python --template-mode --template-data package_name=llm_extension --template-data class_name_prefix
=LLMExtension
```

## Abstract APIs to implement

### on_data_chat_completion(self, ten_env: TenEnv, **kargs: LLMDataCompletionArgs) -> None

This method is called when the LLM Extension receives a data completion request. It's used when data is passed in via data protocol in streaming mode.

### on_call_chat_completion(self, ten_env: TenEnv, **kargs: LLMCallCompletionArgs) -> any

This method is called when the LLM Extension receives a call completion request. It's used when data is passed in via call protocol in non-streaming mode.

This method is called when the LLM Extension receives a call completion request.

### on_tools_update(self, ten_env: TenEnv, tool: LLMToolMetadata) -> None

This method is called when the LLM Extension receives a tool update request.

## APIs

### cmd_in: tool_register

This API is used to consume the tool registration request. An array of LLMToolMetadata will be received as input. The tools will be appended to `self.available_tools` for future use.

### cmd_out: tool_call

This API is used to send the tool call request. You can connect this API to any LLMTool extension destination to get the tool call result.
