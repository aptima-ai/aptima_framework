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

# Customize your agent


## Using the power of TEN Graph Designer(Beta)

<figure><img src="../assets/gif/graph_designer.gif" alt=""><figcaption></figcaption></figure>

TEN Graph Designer is a powerful tool that allows you to customize the behavior and responses of the TEN Agent without needing to write code. This approach is recommended for its ease of use.

In the canvas, you can design your flow by dragging and dropping nodes and connecting them with lines.

Since the TEN Graph Designer is in Beta now, there are some limitations. For example, the graph you are working on needs to match the graph you are testing with. Otherwise, the changes you make will not be reflected in the results.

## Changing the code yourself

If you feel comfortable editing the code yourself, you are more than welcome to do so. In `agents/property.json`, find the corresponding graph and manipulate any values you want.

After making changes, simply refresh the page and connect to the agent, and the changes will take effect.
