# Configure Modules in Playground

This guide will help you to configure modules in the APTIMA-Agent Playground.

## Configure Modules

1. Open the playground at [localhost:3000](http://localhost:3000) to configure your agent.
2. Select a graph type (e.g. Voice Agent, Realtime Agent).
3. Click on the Button to the right of the graph selection to open the module selection.
4. Depending on the graph type, you can select available modules from the dropdown list.
5. Click on `Save Change` to apply the module to the graph.
6. If you see the success toast, the module is successfully applied to the graph.

## Available Modules

The following module types are available for the APTIMA-Agent Playground:

### Speech Recognition (STT)

The Speech Recognition module converts spoken language into text.

### Text-to-Speech (TTS)

The Text-to-Speech module converts text into spoken language.

### Large Language Model (LLM)

The Large Language Model module generates text based on the input text with influence.

### Voide to Voice Model (V2V)

The Voice to Voice Model module generates voice based on the input voice with influence.

### Tool (TOOL)

The Tool module provides a set of tools for the agent to use. The tools can be binded to `LLM` module or `V2V` module.

## Bind Tool Modules

You can bind tool modules to `LLM` or `V2V` modules in the APTIMA-Agent Playground.
Tool can provide additional capabilities to the agent, such as weather check, news update, etc. You can also write your own tool extension if needed.

1. Open the playground at [localhost:3000](http://localhost:3000) to configure your agent.
2. Select a graph type (e.g. Voice Agent, Realtime Agent).
3. Click on the Button to the right of the graph selection to open the module selection.
4. Depending on the graph type, you will see `LLM` or `V2V` module in the Module Picker.
5. Click on the button to the right of the module to open the tool selection.
6. Select a tool available to bind to the module.
7. Click on `Save Change` to apply the tool to the module.
8. If you see the success toast, the tool is successfully applied to the module.
