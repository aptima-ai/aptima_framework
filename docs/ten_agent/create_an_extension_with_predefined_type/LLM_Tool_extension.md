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

# Create a LLM Tool Extension

## Creating AsyncLLMToolBaseExtension by using tman

Run the following command,

```bash
tman install extension default_async_llm_tool_extension_python --template-mode --template-data package_name=llm_tool_extension --template-data class_name_prefix
=LLMToolExtension
```

## Abstract APIs to implement

### get_tool_metadata(self, ten_env: TenEnv) -> list[LLMToolMetadata]

This method is called when the LLM Extension is going to register itself to any connected LLM. It should return a list of LLMToolMetadata.

### run_tool(self, ten_env: AsyncTenEnv, name: str, args: dict) -> LLMToolResult

This method is called when the LLM Extension receives a tool call request. It should execute the tool function and return the result.

## APIs

### cmd_out: tool_register

This API is used to send the tool registration request. An array of LLMToolMetadata returned from `get_tool_metadata` will be sent as output.

### cmd_in: tool_call

This API is used to consume the tool call request. The `run_tool` will be executed when the cmd_in is received.
