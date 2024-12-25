# Understand extension folder

The `ten_packages/extension` folder contains the extension modules. Each extension module is a separate Python/Golang/C++ package.

The extension folder name is often the same as the extension module name, while you can also find the extension module name in the `manifest.json` file under the extension folder. The module name shall be used in the `addon` property of the `property.json` file.

Below is a sample structure of the extension folder:

![Extension Folder Structure](https://github.com/TEN-framework/docs/blob/main/assets/png/extension_folder_struct.png?raw=true)

## Extension Common Files

- manifest.json: This file contains the metadata of the extension. It includes the extension name, version, properties, and apis (data, audio_frame, video_frame).
- property.json: This file contains the default properties of the extension. It is used to define the default configuration of the extension.

## Python Extension

- extension.py: This file contains the main logic of the extension. It usually contains the core implementation of the extension.
- requirements.txt: This file contains the Python dependencies required by the extension. Dependencies specified in this file will be installed automatically when you run `task use`.

## Golang Extension

- <xxx>_extension.go: This file contains the main logic of the extension. It usually contains the core implementation of the extension.
- go.mod: This file contains the Go module definition. It specifies the module name and the dependencies of the extension.

## C++ Extension

- src/main.cc: This file contains the main logic of the extension. It usually contains the core implementation of the extension.
- BUILD.gn: This file contains the build configuration of the extension. It specifies the target name and dependencies of the extension.