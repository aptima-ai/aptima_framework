# Overview

The directory structure of a basic TEN framework software package is shown below.

```text
.
├── bin/
├── src/
├── manifest.json
├── manifest-lock.json
├── property.json
└── ten_packages/
    ├── extension/
    │   ├── <extension_foo>/
    │   └── <extension_bar>/
    ├── extension_group/
    │   └── <extension_group_x>/
    ├── protocol/
    │   └── <protocol_a>/
    └── system/
        ├── <system_package_1>/
        └── <system_package_2>/
```

Depending on the programming language, there may be additional required files. For example, in a C++ project, since the build system is `ten_gn`, a `BUILD.gn` file would be present. In a Go project, there would be files such as `go.mod` and `go.sum`. In a Python project, you might find a `requirements.txt` file.

## TEN Framework Software Package Types

| Type            | Description                     |
|-----------------|---------------------------------|
| App             | Contains a TEN app.             |
| Extension group | Contains a TEN extension group. |
| Extension       | Contains a TEN extension.       |
| Protocol        | Contains a TEN protocol.        |
| System          | Contains a TEN system package.  |

## TEN Framework Software Package Kinds

There are two types of software packages:

- Release package
- Development package

The relationship between these two package types is as follows:

- A release package can be generated from a development package through a build process.
- If we add source-related content to a release package, it becomes a development package.
- If we remove source-related content from a development package, it becomes a release package.

In short, the directory structures of development and release packages are very similar, with the difference being that a development package includes source-related content, whereas a release package does not.

### Development package

The primary purpose of a development package is to develop TEN software packages. For instance, if you want to create a new TEN extension, you can modify an existing development package to achieve this. This is because the development package contains all the source-related content, enabling the creation of new TEN software packages from it.

### Release package

The primary purpose of a release package is to create and run graphs. Since no build tasks are required, a release package can be used to perform these actions. However, a development package can also be used for these tasks. Essentially, the application scope of a development package is broader than that of a release package.
