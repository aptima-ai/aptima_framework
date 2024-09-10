# Extension

## Interface with TEN Runtime

Extensions interact with the TEN runtime primarily through three interfaces:

1. **Lifecycle Callbacks**
   - These include callbacks like `on_init`, `on_deinit`, `on_start`, and `on_stop`.

2. **Callbacks for Receiving Messages**
   - These include callbacks such as `on_cmd`, `on_data`, `on_audio_frame`, and `on_video_frame`, which handle incoming messages to the extension.

3. **Functions for Sending Messages**
   - These include functions like `send_cmd`, `send_data`, `send_audio_frame`, and `send_video_frame`, which send messages out of the extension.

## Lifecycle Callbacks

The different stages of the extension's lifecycle and their connection to message handling are as follows:

- **on_init ~ on_init_done**: Handles the extension's own initialization. During this phase, it cannot send or receive messages.

  - Once all extensions have completed `on_init_done`, they collectively transition to `on_start`.

- **on_start ~ on_start_done**: At this stage, the extension can send messages and receive responses to the messages it sends, but it cannot receive unsolicited messages. Since properties are initialized during `on_start`, you can perform actions that depend on these properties being set up. However, as this is still part of the initialization phase, the extension will not receive messages initiated by others, avoiding the need for various checks. Active message sending is allowed.

- **After on_start_done ~ on_stop_done**: During this phase, the extension can normally send and receive all types of messages and their results.

  - Once all extensions have completed `on_stop_done`, they collectively transition to `on_deinit`.

- **on_deinit ~ on_deinit_done**: Handles the extension's de-initialization. During this phase, it cannot send or receive messages.

## Implementing Extensions in Different Languages

Within the TEN framework, extensions can be implemented in various languages such as C++, Go, and Python. Developers can use the same conceptual approach to implement extensions in different languages. Learning how to develop an extension in one language makes it relatively easy to do so in other languages as well.

## Asynchronous Message Processing in Extensions

<figure><img src="../../assets/png/asynchronous_message_processing.png" alt=""><figcaption><p>Asynchronous Message Processing</p></figcaption></figure>

Extensions process messages asynchronously. When the TEN runtime delivers a message to an extension through callbacks like `on_cmd`, `on_data`, `on_audio_frame`, or `on_video_frame`, the extension is not required to process the message immediately within the callback. Instead, the extension can delegate the message to other threads, processes, or even machines for processing. This allows for full utilization of multi-core and distributed computing resources.

After processing is complete, the results can be sent back to the TEN runtime through callbacks such as `send_cmd`, `send_data`, `send_audio_frame`, or `send_video_frame`. The entire process is asynchronous, meaning the extension doesn't need to send the processed results back before the `on_cmd`, `on_data`, `on_audio_frame`, or `on_video_frame` callbacks return. The results can be transmitted only when they are actually ready, using the appropriate send functions.
