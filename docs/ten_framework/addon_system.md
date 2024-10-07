# Addon System

The TEN framework is a software module system, where each software module is defined by the concept of an **addon**. Essentially, every software module in the TEN framework is an addon. For example, users can define the following types of TEN framework addons:

1. TEN **extension** addon
2. TEN **extension group** addon
3. TEN **protocol** addon

## Information Required to Load Addon

To load an addon into the TEN runtime, three pieces of information are needed:

1. **Addon name**

   It must be globally unique within the entire TEN app. Loading multiple addons with the same name into the TEN runtime is undefined behavior, and it may result in the second instance of the addon not being properly loaded into the TEN runtime.

2. **Addon folder location**

   In the TEN framework, when loading an addon, users must inform the TEN runtime where the addon's directory is located. This directory serves as the addon's `base_dir`. The TEN runtime will use this base directory to load other files related to the addon, such as the addon's `manifest.json` and `property.json` files.

   Basically, when designing the TEN framework's addon register API, it aims to automatically detect the directory where the addon is located, so the user does not need to explicitly specify the `base_dir`. However, in some special cases where the TEN framework cannot automatically detect the `base_dir`, the user will need to explicitly specify the folder where the addon is located for the TEN runtime.

3. **Addon instance itself**

## Timing for Registering Addon to TEN Runtime

There are two ways to register an addon into the TEN runtime:

1. Explicitly call the API to register the addon's `base_dir` to the TEN runtime.

   **Note** Currently, only the standalone testing framework provides such an API.

2. Place the addon in a designated location within the TEN app folder tree.

By placing the addon in the specified location, the TEN app will automatically load these addons when it starts. Currently, this designated location is under the `ten_packages/` directory in the TEN app. If the addons are placed in this format, they will be automatically loaded when the TEN app starts.

```text
.
└── ten_packages/
    ├── extension/
    │   ├── <extension_foo>/
    │   └── <extension_bar>/
    ├── extension_group/
    │   └── <extension_group_x>/
    └── protocol/
        └── <protocol_a>/
```
