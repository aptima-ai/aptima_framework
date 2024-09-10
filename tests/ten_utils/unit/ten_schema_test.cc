//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "gtest/gtest.h"
#include "include_internal/ten_utils/schema/schema.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/value/value.h"
#include "ten_utils/value/value_get.h"
#include "ten_utils/value/value_is.h"
#include "ten_utils/value/value_json.h"
#include "ten_utils/value/value_object.h"

static ten_schema_t *create_ten_schema_from_string(
    const std::string &schema_str) {
  auto *schema_json = ten_json_from_string(schema_str.c_str(), nullptr);
  auto *schema = ten_schema_create_from_json(schema_json);

  ten_json_destroy(schema_json);

  return schema;
}

TEST(SchemaTest, ValidStringType) {  // NOLINT
  const std::string schema_str =
      R"({
           "type": "string"
         })";

  auto *schema = create_ten_schema_from_string(schema_str);

  ten_error_t err;
  ten_error_init(&err);

  auto *str_value = ten_value_create_string("demo");

  bool success = ten_schema_validate_value(schema, str_value, &err);
  ASSERT_EQ(success, true);

  ten_value_destroy(str_value);

  auto *int_value = ten_value_create_int8(1);

  success = ten_schema_validate_value(schema, int_value, &err);
  ASSERT_EQ(success, false);
  ASSERT_EQ(ten_error_is_success(&err), false);

  ten_value_destroy(int_value);

  ten_error_deinit(&err);
  ten_schema_destroy(schema);
}

TEST(SchemaTest, ValidObjectType) {  // NOLINT
  const std::string schema_str =
      R"({
           "type": "object",
           "properties": {
             "name": {
               "type": "string"
             },
             "age": {
               "type": "int64"
             }
           }
         })";
  auto *schema = create_ten_schema_from_string(schema_str);

  const std::string value_str = R"({
                                   "name": "demo",
                                   "age": 18
                                 })";
  auto *value_json = ten_json_from_string(value_str.c_str(), nullptr);
  auto *value = ten_value_from_json(value_json);

  ten_json_destroy(value_json);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_validate_value(schema, value, &err);
  ASSERT_EQ(success, true);

  ten_value_destroy(value);

  ten_error_deinit(&err);
  ten_schema_destroy(schema);
}

TEST(SchemaTest, ValidArrayType) {  // NOLINT
  const std::string schema_str =
      R"({
           "type": "array",
           "items": {
             "type": "int64"
           }
         })";
  auto *schema = create_ten_schema_from_string(schema_str);

  const std::string value_str = R"([1, 2, 3])";
  auto *value_json = ten_json_from_string(value_str.c_str(), nullptr);
  auto *value = ten_value_from_json(value_json);

  ten_json_destroy(value_json);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_validate_value(schema, value, &err);
  ASSERT_EQ(success, true);

  ten_value_destroy(value);

  ten_error_deinit(&err);
  ten_schema_destroy(schema);
}

TEST(SchemaTest, AdjustIntValue) {  // NOLINT
  const std::string schema_str =
      R"({
           "type": "int64"
         })";
  auto *schema = create_ten_schema_from_string(schema_str);

  auto *value = ten_value_create_int8(1);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_adjust_value_type(schema, value, &err);
  ASSERT_EQ(success, true);

  ASSERT_TRUE(ten_value_is_int64(value));
  ASSERT_EQ(1, ten_value_get_int64(value, &err));

  ten_value_destroy(value);

  ten_error_deinit(&err);
  ten_schema_destroy(schema);
}

TEST(SchemaTest, AdjustObject) {  // NOLINT
  const std::string schema_str =
      R"({
           "type": "object",
           "properties": {
             "name": {
               "type": "string"
             },
             "age": {
               "type": "uint8"
             }
           }
         })";
  auto *schema = create_ten_schema_from_string(schema_str);

  const std::string value_str = R"({
                                   "name": "demo",
                                   "age": 18
                                 })";
  auto *value_json = ten_json_from_string(value_str.c_str(), nullptr);
  auto *value = ten_value_from_json(value_json);

  ten_json_destroy(value_json);

  ten_error_t err;
  ten_error_init(&err);

  auto *value_age = ten_value_object_peek(value, "age");
  ASSERT_EQ(true, ten_value_get_uint8(value_age, &err) == 18);

  bool success = ten_schema_adjust_value_type(schema, value, &err);
  ASSERT_EQ(success, true);

  ASSERT_TRUE(ten_value_is_uint8(value_age));
  ASSERT_EQ(18, ten_value_get_uint8(value_age, &err));

  ten_value_destroy(value);

  ten_error_deinit(&err);
  ten_schema_destroy(schema);
}

TEST(SchemaTest, AdjustArray) {  // NOLINT
  const std::string schema_str =
      R"({
           "type": "array",
           "items": {
             "type": "int32"
           }
         })";
  auto *schema = create_ten_schema_from_string(schema_str);

  const std::string value_str = R"([1, 2, 3])";
  auto *value_json = ten_json_from_string(value_str.c_str(), nullptr);
  auto *value = ten_value_from_json(value_json);

  ten_json_destroy(value_json);

  ten_error_t err;
  ten_error_init(&err);

  auto *value_one = ten_value_array_peek(value, 0);
  ASSERT_EQ(true, ten_value_get_int32(value_one, &err) == 1);

  bool success = ten_schema_adjust_value_type(schema, value, &err);
  ASSERT_EQ(success, true);

  ASSERT_EQ(true, ten_value_is_int32(value_one));
  ASSERT_EQ(1, ten_value_get_int32(value_one, &err));

  ten_value_destroy(value);

  ten_error_deinit(&err);
  ten_schema_destroy(schema);
}

TEST(SchemaTest, Required) {  // NOLINT
  const std::string schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint8"
             }
           },
           "required": ["a"]
         })";
  auto *schema = create_ten_schema_from_string(schema_str);

  const std::string value_str = R"({
                                     "b": 18
                                   })";
  auto *value_json = ten_json_from_string(value_str.c_str(), nullptr);
  auto *value = ten_value_from_json(value_json);

  ten_json_destroy(value_json);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_adjust_value_type(schema, value, &err);
  ASSERT_EQ(success, true);

  success = ten_schema_validate_value(schema, value, &err);
  ASSERT_EQ(success, false);

  ten_value_destroy(value);

  ten_error_deinit(&err);
  ten_schema_destroy(schema);
}

TEST(SchemaTest, CompatibleIntSuccess) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "int32"
         })";

  const std::string target_schema_str =
      R"({
           "type": "int64"
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, true);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatibleIntFail) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "int32"
         })";

  const std::string target_schema_str =
      R"({
           "type": "string"
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, false);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatibleProperties) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint8"
             }
           }
         })";

  const std::string target_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint16"
             }
           }
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, true);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatiblePropertiesSuperSet) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             }
           }
         })";

  const std::string target_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint16"
             }
           }
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, true);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatiblePropertiesSubset) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint16"
             }
           }
         })";

  const std::string target_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             }
           }
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, true);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatiblePropertiesMismatchType) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint16"
             }
           }
         })";

  const std::string target_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "int8"
             },
             "c": {
               "type": "uint8"
             }
           }
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, false);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatibleRequired) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint8"
             }
           },
           "required": ["a"]
         })";

  const std::string target_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint16"
             }
           },
           "required": ["a"]
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, true);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatibleRequiredSubset) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint8"
             }
           },
           "required": ["a"]
         })";

  const std::string target_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint16"
             }
           },
           "required": ["a", "b"]
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, false);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatibleRequiredSuperset) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint8"
             }
           },
           "required": ["a", "b"]
         })";

  const std::string target_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint16"
             }
           },
           "required": ["a"]
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, true);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatibleRequiredSourceUndefined) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint8"
             }
           }
         })";

  const std::string target_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint16"
             }
           },
           "required": ["a"]
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, false);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatibleRequiredTargetUndefined) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint8"
             }
           },
           "required": ["a"]
         })";

  const std::string target_schema_str =
      R"({
           "type": "object",
           "properties": {
             "a": {
               "type": "string"
             },
             "b": {
               "type": "uint16"
             }
           }
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, true);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatibleItems) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "array",
           "items": {
             "type": "int32"
           }
         })";

  const std::string target_schema_str =
      R"({
           "type": "array",
           "items": {
             "type": "int64"
           }
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, true);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}

TEST(SchemaTest, CompatibleItemsFail) {  // NOLINT
  const std::string source_schema_str =
      R"({
           "type": "array",
           "items": {
             "type": "int32"
           }
         })";

  const std::string target_schema_str =
      R"({
           "type": "array",
           "items": {
             "type": "int8"
           }
         })";

  auto *source_schema = create_ten_schema_from_string(source_schema_str);
  auto *target_schema = create_ten_schema_from_string(target_schema_str);

  ten_error_t err;
  ten_error_init(&err);

  bool success = ten_schema_is_compatible(source_schema, target_schema, &err);
  ASSERT_EQ(success, true);

  ten_error_deinit(&err);

  ten_schema_destroy(source_schema);
  ten_schema_destroy(target_schema);
}
