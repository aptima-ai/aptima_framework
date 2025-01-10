# Build System

## Build System

### C++

The APTIMA framework uses the `ten_gn` build system to compile C++ apps and C++ extensions. `ten_gn` is a build system based on Google's GN.

### Golang

The APTIMA framework uses the standard go command to build APTIMA framework Golang projects.

## Building

### C++ App

To build a APTIMA framework C++ app, run the following commands in the root directory of the APTIMA framework C++ app.

`<os>` can be one of the following:

- linux
- mac
- win

`<arch>` can be one of the following:

- x86
- x64
- arm
- arm64

`<build_type>` can be one of the following:

- debug
- release

```shell
ten_gn gen <os> <arch> <build_type>
ten_gn build <os> <arch> <build_type>
```

### C++ Addon

The term `addon` here includes the following:

- extension
- extension group
- protocol

The method to build APTIMA framework C++ addons is the same as for building APTIMA framework C++ apps.

### Golang App

To build a APTIMA Golang project, run the following command in the root directory of the APTIMA Golang project:

```shell
go run ten_packages/system/ten_runtime_go/tools/build/main.go
```
