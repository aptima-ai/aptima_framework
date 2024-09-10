# Type System

## Type

The TEN framework type system is the system used within the TEN framework to define the data types of values. Developers can declare the types of message or extension properties through the TEN schema.

The TEN framework type system includes basic types and composite types. The basic types include the following:

| Type    | Description                                               | C++ Type      | Go Type | Python Type        |
|---------|-----------------------------------------------------------|---------------|---------|--------------------|
| int8    | An 8-bit signed integer.                                   | int8_t        | int8    | int                |
| int16   | A 16-bit signed integer.                                   | int16_t       | int16   | int                |
| int32   | A 32-bit signed integer.                                   | int32_t       | int32   | int                |
| int64   | A 64-bit signed integer.                                   | int64_t       | int64   | int                |
| uint8   | An 8-bit unsigned integer.                                 | uint8_t       | uint8   | int                |
| uint16  | A 16-bit unsigned integer.                                 | uint16_t      | uint16  | int                |
| uint32  | A 32-bit unsigned integer.                                 | uint32_t      | uint32  | int                |
| uint64  | A 64-bit unsigned integer.                                 | uint64_t      | uint64  | int                |
| float32 | A single precision (32-bit) IEEE 754 floating-point number.| float         | float32 | float              |
| float64 | A double-precision (64-bit) IEEE 754 floating-point number.| double        | float64 | float              |
| string  | A Unicode character sequence.                              | std::string / char\* | string  | str        |
| buf     | A sequence of 8-bit unsigned bytes.                        | uint8_t\*     | \[\]byte | bytearray / memoryview |
| bool    | A binary value, either true or false.                      | bool          | bool    | bool               |
| ptr     | A pointer to a memory address.                             | void\*        | unsafe.Pointer |                 |

Composite types include the following:

| Type   | Description                                      | C++ Type | Go Type | Python Type |
|--------|--------------------------------------------------|----------|---------|-------------|
| array  | A collection of elements of the same type.       ||||
| object | Represents a complex key/value pair. The key type is always a string. ||||

For basic types, properties can be accessed or set using methods such as `get_property()` / `set_property()`. For example:

```cpp
// Get property value
int32_t value = cmd.get_property_int32("property_name");

// Set property value
cmd.set_property("property_name", 100);
```

For composite types, it is typically necessary to use related serialization methods. For example:

```Golang
type MyProp struct {
    Name string `json:"name"`
}

var prop MyProp
bytes, _ := json.Marshal(&prop)
cmd.SetPropertyFromJSONBytes("property_name", bytes)
```

## Type and Schema

If a TEN schema is specified, the property type will be determined according to the corresponding TEN schema. If no TEN schema is specified, the property type will be determined based on the type of the initial value assignment. For example, if the initial assignment is `int32_t`, the property type will be `int32_t`; if the initial assignment is done using JSON, the type will be determined according to the JSON processing rules.

## Conversion Rules

The TEN framework supports flexible automatic conversion between different types of values. As long as the conversion does not result in a loss of value, the TEN framework will automatically perform the type conversion. However, if the conversion leads to a loss of value, such as converting a type from `int32_t` to `int8_t` when the value exceeds the range that `int8_t` can represent, the TEN framework will report an error. For example, the TEN framework will return an error for a `send_<foo>` action.

### Safe and Must-Succeed Conversion

Converting a lower precision type to a higher precision type is always safe and guaranteed to succeed, as the higher precision type can fully accommodate the value of the lower precision type without data loss. This automatic conversion is called Safe Conversion. For example, when trying to retrieve an `int8` type property as `int32`, the TEN type system will automatically convert the property type to `int32`.

In the TEN Type System, the Safe Conversion rules are as follows:

1. Within the `int` types, from lower to higher precision.
2. Within the `uint` types, from lower to higher precision.
3. Within the `float` types, from lower to higher precision.

| From    | To (Allowed)             |
|---------|--------------------------|
| int8    | int16 / int32 / int64    |
| int16   | int32 / int64            |
| int32   | int64                    |
| uint8   | uint16 / uint32 / uint64 |
| uint16  | uint32 / uint64          |
| uint32  | uint64                   |
| float32 | float64                  |

For example:

```cpp
// Set property value. The type of `property_name` in TEN Runtime is `int32`.
cmd.set_property("property_name", 100);

// Get property value. Correct.
int32_t value = cmd.get_property_int32("property_name");

// Get property value. Correct. TEN Type System will automatically convert the type to `int64`.
int64_t value2 = cmd.get_property_int64("property_name");

// Get property value. Incorrect, an error will be thrown.
int16_t error_type = cmd.get_property_int16("property_name");
```

### Unsafe and Might-Fail Conversion

Converting a higher precision type to a lower precision type is unsafe because the value of the higher precision type may exceed the range of the lower precision type, leading to data loss. This conversion is called Unsafe Conversion. When performing Unsafe Conversion, the TEN runtime checks for overflow. If an overflow occurs, the TEN Type System will throw an error.

In the TEN framework type system, the Unsafe Conversion rules are as follows:

1. Converting `int64` to a lower precision `int`.
2. Converting `int64` to any precision `uint`.
3. Converting `float64` to `float32`.

| From    | To      | Correct Value Range of From                              |
|---------|---------|----------------------------------------------------------|
| int64   | int8    | \[-2^7, 2^7 - 1\]                                         |
| int64   | int16   | \[-2^15, 2^15 - 1\]                                       |
| int64   | int32   | \[-2^31, 2^31 - 1\]                                       |
| int64   | uint8   | \[0, 2^7 - 1\]                                            |
| int64   | uint16  | \[0, 2^15 - 1\]                                           |
| int64   | uint32  | \[0, 2^31 - 1\]                                           |
| int64   | uint64  | \[0, 2^63 - 1\]                                           |
| float64 | float32 | \[-3.4028234663852886e+38, 3.4028234663852886e+38\]       |

It is important to note that TEN runtime only performs Unsafe Conversion when deserializing a JSON document into a TEN property and the TEN property has a defined TEN schema. For example:

- When loading `property.json`.

  For integers, they will be parsed as `int64` by default; for floating-point numbers, they will be parsed as `float64` by default. The TEN framework type system will perform Unsafe Conversion according to the rules mentioned above.

- When calling methods such as `set_property_from_json()`.

  When passing a serialized JSON string, the TEN framework type system will also perform Unsafe Conversion according to the rules mentioned above.
