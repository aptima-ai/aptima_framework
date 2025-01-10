# Development Process

Basically, by using the APTIMA framework to complete the desired scenarios, there are two development approaches or perspectives:

1. From small extensions to large scenarios
2. From large scenarios to small extensions

**Note:** Regardless of which of these two development approaches you take, you can either write your own extensions from scratch or download pre-built extensions from the APTIMA cloud store.

In the first approach, **from small extensions to large scenarios**, the development process typically follows these steps:

1. Develop extensions.
2. Assemble the extensions into a graph, and further create an agent, for example, [APTIMA-Agent](https://github.com/APTIMA-framework/APTIMA-Agent).

In the second approach, **from large scenarios to small extensions**, the development process typically follows these steps:

1. Select the desired agent template, for example, [APTIMA-Agent](https://github.com/APTIMA-framework/APTIMA-Agent).
2. Replace some of the extensions within the chosen agent template with your own.

In this second development approach, you can ensure that the behavior of the replaced extension matches the original one by using the framework for **standalone testing** of extensions. In this process, the developer only needs to understand concepts related to APTIMA extensions, including how to independently develop and test a APTIMA extension. All other APTIMA-related concepts outside of extension development and testing do not need to be understood, as these concepts are encapsulated within the selected APTIMA agent.
