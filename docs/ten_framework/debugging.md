# Debugging

## For Developers of the TEN Framework

If you're using VS Code to develop the TEN framework, there is a `launch.json` file in the TEN framework source tree that provides default debug targets.

## For Users of the TEN Framework

If you're using VS Code to develop your own application based on the TEN framework, you can add configurations to your `.vscode/launch.json` file so that you can debug your application with breakpoints and variable inspection whichever programming language (C++/Golang/Python) you are using.

### Debugging in C++ Applications

If the application is written in C++, it means the extensions can be written
in C++ or Python.

#### Debugging C++ code with lldb or gdb

You can use the following configurations to debug C++ code:

* **Debugging C++ code with lldb**

  ```json
  {
      "name": "app (C++) (lldb, launch)",
      "type": "lldb",
      "request": "launch", // "launch" or "attach"
      "program": "${workspaceFolder}/bin/worker", // The executable path
      "cwd": "${workspaceFolder}/",
      "env": {
          "LD_LIBRARY_PATH": "${workspaceFolder}/ten_packages/system/xxx/lib", // linux
          "DYLD_LIBRARY_PATH": "${workspaceFolder}/ten_packages/system/xxx/lib" // macOS
      }
  }
  ```

* **Debugging C++ code with gdb**

  ```json
  {
      "name": "app (C++) (gdb, launch)",
      "type": "cppdbg",
      "request": "launch", // "launch" or "attach"
      "program": "${workspaceFolder}/bin/worker", // The executable path
      "cwd": "${workspaceFolder}/",
      "MIMode": "gdb",
      "environment": [
          {
              // linux
              "name": "LD_LIBRARY_PATH",
              "value": "${workspaceFolder}/ten_packages/system/xxx/lib"
          },
          {
              // macOS
              "name": "DYLD_LIBRARY_PATH",
              "value": "${workspaceFolder}/ten_packages/system/xxx/lib"
          }
      ]
  }
  ```

#### Debugging Python code with debugpy

You can use `debugpy` to debug Python code. However, since the program is not started directly through the Python interpreter, the Python code is executed by the embedded Python interpreter. Therefore, you can only attach a debugger to a running process.

First, you need to install `debugpy` in the Python environment:

```shell
pip install debugpy
```

Then, you need to start the application with environment variable `TEN_ENABLE_PYTHON_DEBUG` and `TEN_PYTHON_DEBUG_PORT` set. For example:

```shell
TEN_ENABLE_PYTHON_DEBUG=true TEN_PYTHON_DEBUG_PORT=5678 ./bin/worker
```

If the environment variable `TEN_ENABLE_PYTHON_DEBUG` is set to `true`, then the application will block until the debugger is attached. If the environment variable `TEN_PYTHON_DEBUG_PORT` is set, then the debugger server will listen on the specified port for incoming connections.

Then, you can attach the debugger to the running process. Here is an example of the configuration:

**Debugging Python code with debugpy**

```json
{
    "name": "app (C++) (debugpy, attach)",
    "type": "debugpy",
    "request": "attach",
    "connect": {
        "host": "localhost",
        "port": 5678
    },
    "justMyCode": false
}
```

#### Debugging C++ and Python code at the same time

If you want to start debugging C++ and Python code at the same time with one click in VSCode, you can do the following:

1. Define a task for delaying several seconds before attaching the debugger:

   ```json
   {
      "label": "delay 3 seconds",
      "type": "shell",
      "command": "sleep 3",
      "windows": {
        "command": "ping 127.0.0.1 -n 3 > nul"
      },
      "group": "none",
   }
   ```

2. Add the following configurations to the `launch.json` file:

   ```json
   "configurations": [
        {
            "name": "app (C++) (lldb, launch with debugpy)",
            "type": "lldb",
            "request": "launch", // "launch" or "attach"
            "program": "${workspaceFolder}/bin/worker", // The executable path
            "cwd": "${workspaceFolder}/",
            "env": {
                "LD_LIBRARY_PATH": "${workspaceFolder}/ten_packages/system/xxx/lib", // linux
                "DYLD_LIBRARY_PATH": "${workspaceFolder}/ten_packages/system/xxx/lib", // macOS
                "TEN_ENABLE_PYTHON_DEBUG": "true",
                "TEN_PYTHON_DEBUG_PORT": "5678"
            }
        },
        {
            "name": "app (C++) (debugpy, attach with delay)",
            "type": "debugpy",
            "request": "attach",
            "connect": {
                "host": "localhost",
                "port": 5678
            },
            "justMyCode": false,
            "preLaunchTask": "delay 3 seconds"
        }
   ],
   "compounds": [
        {
            "name": "app (C++) (lldb, launch) and app (C++) (debugpy, attach)",
            "configurations": [
                "app (C++) (lldb, launch with debugpy)",
                "app (C++) (debugpy, attach with delay)"
            ]
        }
   ]
   ```

Then, you can start debugging C++ and Python code with the compound configuration `app (C++) (lldb, launch) and app (C++) (debugpy, attach)`.

### Debugging in Go applications

If the application is written in Go, it means the extensions can be written in Go or C++ or Python.

#### Debugging Go code with delve

You can use the following configuration to debug in your Go extensions:

```json
{
    "name": "app (golang) (go, launch)",
    "type": "go",
    "request": "launch",
    "mode": "exec",
    "cwd": "${workspaceFolder}/",
    "program": "${workspaceFolder}/bin/worker", // The executable path
    "env": {
        "LD_LIBRARY_PATH": "${workspaceFolder}/ten_packages/system/xxx/lib", // linux
        "DYLD_LIBRARY_PATH": "${workspaceFolder}/ten_packages/system/xxx/lib", // macOS
        "TEN_APP_BASE_DIR": "${workspaceFolder}",
    }
}
```

#### Debugging C++ code with lldb or gdb

You can use the following configurations to debug C++ code:

* **debugging C++ code with lldb**

  ```json
  {
      "name": "app (Go) (lldb, launch)",
      "type": "lldb",
      "request": "launch", // "launch" or "attach"
      "program": "${workspaceFolder}/bin/worker", // The executable path
      "cwd": "${workspaceFolder}/",
      "env": {
          "LD_LIBRARY_PATH": "${workspaceFolder}/ten_packages/system/xxx/lib", // linux
          "DYLD_LIBRARY_PATH": "${workspaceFolder}/ten_packages/system/xxx/lib" // macOS
      },
      "initCommands": [
          "process handle SIGURG --stop false --pass true"
      ]
  }
  ```

* **debugging C++ code with gdb**

  ```json
  {
      "name": "app (Go) (gdb, launch)",
      "type": "cppdbg",
      "request": "launch", // "launch" or "attach"
      "program": "${workspaceFolder}/bin/worker", // The executable path
      "cwd": "${workspaceFolder}/",
      "MIMode": "gdb",
      "environment": [
          {
              // linux
              "name": "LD_LIBRARY_PATH",
              "value": "${workspaceFolder}/ten_packages/system/xxx/lib"
          },
          {
              // macOS
              "name": "DYLD_LIBRARY_PATH",
              "value": "${workspaceFolder}/ten_packages/system/xxx/lib"
          }
      ]
  }
  ```

#### Debugging Python code with debugpy

Refer to the section above for debugging Python code with debugpy. Start the application with environment variable `TEN_ENABLE_PYTHON_DEBUG` and `TEN_PYTHON_DEBUG_PORT` set.

```shell
TEN_ENABLE_PYTHON_DEBUG=true TEN_PYTHON_DEBUG_PORT=5678 ./bin/worker
```

Then, you can attach the debugger to the running process. Here is an example of the configuration:

```json
{
    "name": "app (C++) (debugpy, attach)",
    "type": "debugpy",
    "request": "attach",
    "connect": {
        "host": "localhost",
        "port": 5678
    },
    "justMyCode": false
}
```

#### Debugging C++, Go and Python code at the same time

If you want to start debugging C++, Go and Python code at the same time with one click in VSCode, you can do the following:

1. Define a task for delaying several seconds before attaching the debugger:

   ```json
   {
      "label": "delay 3 seconds",
      "type": "shell",
      "command": "sleep 3",
      "windows": {
        "command": "ping 127.0.0.1 -n 3 > nul"
      },
      "group": "none",
    }
   ```

2. Add the following configurations to the `launch.json` file:

   ```json
   "configurations": [
        {
            "name": "app (golang) (debugpy, remote attach with delay)",
            "type": "debugpy",
            "request": "attach",
            "connect": {
                "host": "localhost",
                "port": 5678
            },
            "preLaunchTask": "delay 3 seconds",
            "justMyCode": false
        },
        {
            "name": "app (golang) (go, launch with debugpy)",
            "type": "go",
            "request": "launch",
            "mode": "exec",
            "cwd": "${workspaceFolder}/",
            "program": "${workspaceFolder}/bin/worker",
            "env": {
                "TEN_APP_BASE_DIR": "${workspaceFolder}",
                "TEN_ENABLE_PYTHON_DEBUG": "true",
                "TEN_PYTHON_DEBUG_PORT": "5678"
            }
        },
        {
            "name": "app (golang) (lldb, launch with debugpy)",
            "type": "lldb",
            "request": "launch",
            "program": "${workspaceFolder}/bin/worker",
            "cwd": "${workspaceFolder}/",
            "env": {
                "TEN_ENABLE_PYTHON_DEBUG": "true",
                "TEN_PYTHON_DEBUG_PORT": "5678"
            },
            "initCommands": [
                "process handle SIGURG --stop false --pass true"
            ]
        },
   ],
   "compounds": [
        {
            "name": "Mixed Go/Python",
            "configurations": [
                "app (golang) (go, launch with debugpy)",
                "app (golang) (debugpy, remote attach with delay)"
            ],
        },
        {
            "name": "Mixed Go/Python/C++ (lldb)",
            "configurations": [
                "app (golang) (lldb, launch with debugpy)",
                "app (golang) (debugpy, remote attach with delay)"
            ]
        }
   ]
   ```

If you want to debug C++ and Python code at the same time, you can use the compound configuration `Mixed Go/Python/C++ (lldb)`. With this configuration, you can perform breakpoint debugging for C++, Go and Python code simultaneously. However, the variable inspection is only supported for C++ and Python but not for Go code.

If you want to debug Go and Python code at the same time, you can use the compound configuration `Mixed Go/Python`. With this configuration, you can perform breakpoint debugging for Go and Python code simultaneously and inspect variables for both Go and Python code.

### Debugging in Python applications

If the application is written in Python, it means the extensions can be written in Python or C++.

#### Debugging C++ code with lldb or gdb

You can use the following configurations to debug C++ code:

```json
{
    "name": "app (Python) (cpp, launch)",
    "type": "cppdbg", //
    "request": "launch", // "launch" or "attach"
    "program": "/usr/bin/python3", // The Python interpreter path
    "args": [
        "main.py"
    ],
    "cwd": "${workspaceFolder}/",
    "environment": [
        {
          "name": "PYTHONPATH",
          "value": "${workspaceFolder}/ten_packages/system/ten_runtime_python/lib:${workspaceFolder}/ten_packages/system/ten_runtime_python/interface"
        },
        {
          "name": "TEN_APP_BASE_DIR",
          "value": "${workspaceFolder}/"
        },
    ],
    "MIMode": "gdb", // "gdb" or "lldb"
}
```

#### Debugging Python code with debugpy

Refer to the section above for debugging Python code with debugpy. You can use
the following configurations to debug Python code:

```json
{
    "name": "app (Python) (debugpy, launch)",
    "type": "debugpy",
    "python": "/usr/bin/python3",
    "request": "launch",
    "program": "${workspaceFolder}/main.py",
    "console": "integratedTerminal",
    "cwd": "${workspaceFolder}/",
    "env": {
        "PYTHONPATH": "${workspaceFolder}/ten_packages/system/ten_runtime_python/lib:${workspaceFolder}/ten_packages/system/ten_runtime_python/interface",
        "TEN_APP_BASE_DIR": "${workspaceFolder}/",
        "TEN_ENABLE_PYTHON_DEBUG": "true",
    }
}
```

#### Debugging C++ and Python code at the same time

If you want to start debugging C++ and Python code at the same time, you can launch the application with the configuration `app (Python) (debugpy, launch)` and attach the debugger to the running process with the following configuration:

```json
{
    "name": "app (Python) (cpp, attach)",
    "type": "cppdbg",
    "request": "attach", // "launch" or "attach"
    "program": "${workspaceFolder}/bin/worker", // The executable path
    "cwd": "${workspaceFolder}/",
    "MIMode": "gdb",
    "environment": [
        {
            "name": "LD_LIBRARY_PATH",
            "value": "${workspaceFolder}/ten_packages/system/xxx/lib"
        },
        {
            "name": "DYLD_LIBRARY_PATH",
            "value": "${workspaceFolder}/ten_packages/system/xxx/lib"
        }
    ]
}
```
