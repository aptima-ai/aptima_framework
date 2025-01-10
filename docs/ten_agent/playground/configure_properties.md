# Configure Properties in Playground

This guide will help you to configure properties in the APTIMA-Agent Playground.

## Configure Properties

1. Open the playground at [localhost:3000](http://localhost:3000) to configure your agent.
2. Select a graph type (e.g. Voice Agent, Realtime Agent).
3. Click on the Button to the right of the graph selection to open the property configuration.
4. From the dropdown list, you can find all extension nodes used in the graph.
5. Choose an extension node to configure its properties. e.g. stt / llm / tts / v2v / tool.
6. You will see a list of properties that can be configured for the selected extension node.
7. You can change the value of the property by clicking on the input field or switch (if it's a boolean)
8. Click on `Save Change` to apply the property to the extension node.
9. If you see the success toast, the property is successfully applied to the extension node.

## Add more properties

Some properties are defined in the extension node, while does not currently has a value. These properties will not be shown in the property configuration. You can add more properties by following the steps below:

1. In the propety list drawer, click on the `Add Property` button.
2. A new drawer will be opened with a dropdown list of available properties.
3. Select a property from the dropdown list.
4. Click on `Add` to add the property to the extension node.
5. Configure the value of the property.
6. Click on `Save Change` to apply the property to the extension node.
7. If you see the success toast, the property is successfully applied to the extension node.
