# Required

In the APTIMA framework, only message schemas are allowed to have `required` fields, and the properties of extensions are currently not permitted to include `required` fields.

This means that the schemas for `cmd_in`, `cmd_out`, `data_in`, `data_out`, `audio_frame_in`, `audio_frame_out`, `video_frame_in`, and `video_frame_out` are allowed to have `required` fields.

For message schemas, the `required` field can only appear in three specific places:

- At the same level as the `property` in `<foo>_in`/`<foo>_out`.
- Within the `property` of `result` in `<foo>_in`/`<foo>_out`.
- Inside `property` with a type of `object` (i.e., in nested cases).

Examples of these three scenarios are shown below.

```json
{
  "api": {
    "cmd_in": [
      {
        "name": "foo",
        "property": {
          "a": {
            "type": "int8"
          },
          "b": {
            "type": "uint8"
          },
          "c": {
            "type": "array",
            "items": {
              "type": "string"
            }
          },
          "d": {
            "type": "object",
            "properties": {
              "e": {
                "type": "float32"
              }
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
            },
            "required": ["foo"]  // 3.
          }
        },
        "required": ["a", "b"],  // 1.
        "result": {
          "property": {
            "ccc": {
              "type": "buf"
            },
            "detail": {
              "type": "buf"
            }
          },
          "required": ["ccc"]  // 2.
        }
      }
    ]
  }
}
```

## Use of `required`

### When a Message is Sent from an Extension

When extension calls `send_<foo>(msg_X)` or `return_result(result_Y)`, the framework checks `msg_X` or `result_Y` against their respective schemas in extension. If `msg_X` or `result_Y` is missing any of the fields marked as `required` in the schema, the schema check fails, indicating an error.

The handling of these three scenarios is identical, though they are discussed separately:

1. If `send_<foo>` is sending a APTIMA command and the schema check fails:

   `send_<foo>` will return false immediately, and if an error parameter is provided, it will include the schema check failure error message.

2. If `return_result` fails the schema check:

   `return_result` will return false, and if an error parameter is provided, it can include the schema check failure error message.

3. If `send_<foo>` is sending a general data-like APTIMA message (such as data, audio frame, or video frame):

   `send_<foo>` will return false, and if an error parameter is provided, it can include the schema check failure error message.

### When a Message is Received by an Extension

Before ten_runtime passes `msg_X` or `result_Y` to an extension's `on_<foo>()` or result handler, it checks whether all `required` fields defined in the schema of `msg_X` or `result_Y` are present. If any `required` field is missing, the schema check fails.

1. If the incoming message is a APTIMA command:

   ten_runtime will return an error `status_code` result to the previous extension.

2. If the incoming message is a APTIMA command result:

   ten_runtime will change the `status_code` of the result to error, add the missing `required` fields, and set the values of these fields to their default values based on their type.

3. If the incoming message is a APTIMA data-like message:

   ten_runtime will simply drop the data-like message.

## Behavior of Graph Check

APTIMA Manager has a function called Graph Check, which is used to verify the semantic correctness of a graph. The checks related to required fields are as follows:

1. For a connection, the `required` fields of the source must be a superset of the `required` fields of the destination.
2. If the same field name appears in both the source and destination `required` fields, their types must be compatible.
