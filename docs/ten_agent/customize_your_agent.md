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


## Using Playground

<figure><img src="https://github.com/TEN-framework/docs/blob/main/assets/gif/module-example.gif?raw=true" alt=""><figcaption></figcaption></figure>

Once you have the playground running at [ localhost:3000 ](http://localhost:3000), you can customize your agent through three simple steps:

1. Graph Type Selection
   - Choose between Voice Agent, Realtime Agent, or other types

2. Module Selection
   - Pick a module that matches your chosen graph type

3. Extension Configuration
   - Select extensions and configure their API keys
   - Adjust settings as needed

The playground provides an intuitive interface to connect these components without coding.

## Changing the code yourself

If you feel comfortable editing the code yourself, you are more than welcome to do so. In `agents/property.json`, find the corresponding graph and manipulate any values you want.

After making changes, simply refresh the page and connect to the agent, and the changes will take effect.
