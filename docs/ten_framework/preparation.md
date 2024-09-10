# Preparation

The TEN framework supports Windows, Linux, and Mac as development environments, but it is recommended to use Linux or Mac.

## Development Environment

### Linux

Using Ubuntu as an example, you must first install the following packages:

- gcc: One of the C/C++ compilers supported by the TEN framework.
- clang: Another C/C++ compiler supported by the TEN framework.
- clang-format: A code formatting tool used by the TEN framework during runtime.
- clang-tidy: A static code analyzer.
- cmake: The build system. The version must be greater than 3.13.
- Python3: Required for the TEN framework Python binding.
- pytest: Used for the TEN framework's integration testing.

### Mac

To set up your Mac development environment, you need the following:

- XCode
- CocoaPods
- Personal Account

Follow these instructions to prepare the environment:

```shell
brew install llvm googletest doxygen ninja clang-format
brew install include-what-you-use
```

Since the version of `cmake` installed via `brew` might be outdated, if so, you need to install `cmake` from the official website and set up a symbolic link.

```shell
ln -s /Applications/CMake.app/Contents/bin/cmake /usr/local/bin/cmake
```

If `cmake` cannot find `clang-tidy`, you can resolve the issue with the following command:

```shell
ln -sf /usr/local/opt/llvm/bin/clang-tidy /usr/local/bin/clang-tidy
```

To build various language bindings for the TEN framework, you can install the required languages using the following command:

```shell
brew install python golang
```

### Windows

If you need to develop the TEN framework on Microsoft Windows, you can manually install the following packages:

- Visual Studio Community Version (up to 2022)

  Be sure to select the clang-related tools and add the clang binary path to the `PATH` environment variable.

- cmake
- python

> ⚠️ **Note:**
> When installing `cmake`/`python` on Windows, the installation path should not contain spaces.

## ten_gn

The TEN framework uses `ten_gn` as its build system. `ten_gn` is a build system based on Google GN. The source code for `ten_gn` is located in the `core/ten_gn` directory of the TEN framework repository. Add the `ten_gn` directory path to the system `PATH` in your development environment.

## Using Docker Containers

We provide pre-written Docker files that allow you to create a container with all the necessary packages required to build the TEN framework from source.

### Ubuntu 18.04

Navigate to `tools/docker_for_building/ubuntu/18.04` and run the following commands to create and enter the build environment.

```shell
docker-compose up -d
docker-compose run ten-building-ubuntu-1804
```

### Ubuntu 21.10

Follow the same steps as for Ubuntu 18.04, but use the following command to enter the Ubuntu 21.10 build container.

```shell
docker-compose run ten-building-ubuntu-2110
```
