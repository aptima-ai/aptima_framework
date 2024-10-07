# TEN Framework Schema System

## Overview

The TEN framework uses a schema system to define and validate data structures, known as TEN Values, within the TEN runtime. These schemas are used to describe the properties of extensions, as well as the messages exchanged between them. The schemas ensure data consistency, type safety, and proper data handling across different components of the TEN framework.

### Example of a TEN Framework Schema

```json
{
  "api": {
    "property": {
      "exampleInt8": {
        "type": "int8"
      },
      "exampleString": {
        "type": "string"
      }
    },
    "cmd_in": [
      {
        "name": "cmd_1",
        "property": {
          "foo": {
            "type": "int8"
          },
          "bar": {
            "type": "string"
          }
        },
        "result": {
          "property": {
            "x": {
              "type": "int8"
            },
            "y": {
              "type": "string"
            }
          }
        }
      }
    ],
    "cmd_out": [],
    "data_in": [],
    "data_out": [],
    "video_frame_in": [],
    "video_frame_out": [],
    "audio_frame_in": [],
    "audio_frame_out": []
  }
}
```

## Design Principles of the TEN Framework Schema System

1. **Object Principle**
   Every field’s schema in the TEN framework must be defined as an object. This ensures a structured and consistent format across all schema definitions.

   ```json
   {
     "foo": {
       "type": "int8"
     }
   }
   ```

   Incorrect format:

   ```json
   {
     "foo": "int8"
   }
   ```

2. **Metadata-Only Principle**
   The schema defines only metadata, not actual data values. This separation ensures that the schema remains a template for validation and does not mix with data content.

3. **Conflict Prevention Principle**
   In any JSON level containing a TEN schema, all fields must be user-defined, except for reserved fields like `_ten`. This prevents conflicts between user-defined fields and system-defined fields.

   Example with user-defined fields:

   ```json
   {
     "foo": "int8",
     "bar": "string"
   }
   ```

   Example with reserved `_ten` field:

   ```json
   {
     "_ten": {
       "xxx": {}
     },
     "foo": "int8",
     "bar": "string"
   }
   ```

## Defining Types in a TEN Schema

### Primitive Types

The TEN framework supports the following primitive types:

- int8, int16, int32, int64
- uint8, uint16, uint32, uint64
- float32, float64
- string
- bool
- buf
- ptr

Example type definitions:

```json
{
  "foo": {
    "type": "string"
  }
}
```

```json
{
  "foo": {
    "type": "int8"
  }
}
```

### Complex Types

- **Object**

  ```json
  {
    "foo": {
      "type": "object",
      "properties": {
        "foo": {
          "type": "int8"
        },
        "bar": {
          "type": "string"
        }
      }
    }
  }
  ```

- **Array**

  ```json
  {
    "foo": {
      "type": "array",
      "items": {
        "type": "string"
      }
    }
  }
  ```

## Defining the TEN Schema for Properties

### Example Property Schema

```json
{
  "exampleInt8": 10,
  "exampleString": "This is a test string.",
  "exampleArray": [0, 7],
  "exampleObject": {
    "foo": 100,
    "bar": "fine"
  }
}
```

### Corresponding TEN Schema

```json
{
  "api": {
    "property": {
      "exampleInt8": {
        "type": "int8"
      },
      "exampleString": {
        "type": "string"
      },
      "exampleArray": {
        "type": "array",
        "items": {
          "type": "int64"
        }
      },
      "exampleObject": {
        "type": "object",
        "properties": {
          "foo": {
            "type": "int32"
          },
          "bar": {
            "type": "string"
          }
        }
      }
    }
  }
}
```

## Defining the TEN Schema for Commands

### Input Command Example

```json
{
  "_ten": {
    "name": "cmd_foo",
    "seq_id": "123",
    "dest": [{
      "app": "msgpack://127.0.0.1:8001/",
      "graph": "default",
      "extension_group": "group_a",
      "extension": "extension_b"
    }]
  },
  "foo": 3,
  "bar": "hello world"
}
```

### Corresponding TEN Schema

```json
{
  "api": {
    "cmd_in": [
      {
        "name": "cmd_foo",
        "_ten": {
          "name": {
            "type": "string"
          },
          "seq_id": {
            "type": "string"
          },
          "dest": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "app": {
                  "type": "string"
                },
                "graph": {
                  "type": "string"
                },
                "extension_group": {
                  "type": "string"
                },
                "extension": {
                  "type": "string"
                }
              }
            }
          }
        },
        "property": {
          "foo": {
            "type": "int8"
          },
          "bar": {
            "type": "string"
          }
        }
      }
    ]
  }
}
```

To avoid redundancy, the TEN framework allows you to exclude the `_ten` field from your schema definition, as it is reserved and defined by the runtime.

### Defining Command Results

Command results are defined similarly to commands, but are used to describe the expected response:

```json
{
  "api": {
    "cmd_in": [
      {
        "name": "cmd_foo",
        "property": {
          "foo": {
            "type": "int8"
          },
          "bar": {
            "type": "string"
          }
        },
        "result": {
          "property": {
            "aaa": {
              "type": "int8"
            },
            "bbb": {
              "type": "string"
            }
          }
        }
      }
    ]
  }
}
```

## Defining the TEN Schema for Data, Video Frames, and Audio Frames

The process for defining schemas for data, video frames, and audio frames is similar to that for commands but without the result field.

## Manifest Schema Overview

The `manifest.json` file contains the schema definitions for an extension’s properties and messages. These schemas ensure that the extension's configuration and communication follow the correct structure and type requirements.

### Example `manifest.json`

```json
{
  "type": "extension",
  "name": "A",
  "version": "1.0.0",
  "dependencies": [],
  "api": {
    "property": {
      "app_id": {
        "type": "string"
      },
      "channel": {
        "type": "string"
      },
      "log": {
        "type": "object",
        "properties": {
          "level": {
            "type": "uint8"
          },
          "redirect_stdout": {
            "type": "bool"
          },
          "file": {
            "type": "string"
          }
        }
      }
    },
    "cmd_in": [],
    "cmd_out": [],
    "data_in": [],
    "data_out": [],
    "video_frame_in": [],
    "video_frame_out": [],
    "audio_frame_in": [],
    "audio_frame_out": []
  }
}
```

### Conclusion

The TEN framework schema system provides a robust and structured way to define and validate data structures, ensuring consistency and safety across extensions and their interactions within the TEN runtime. By adhering to the principles of object structure, metadata focus, and conflict prevention, the system facilitates clear and effective communication between components.
