//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "axis_utils/value/value.h"

typedef struct axis_schema_t axis_schema_t;

/**
 * @param schemas_content The schema definition contains the 'property' part.
 * Ex:
 * 1) The definition of the property of extension is as follows:
 *
 *		{
 *				"property": {
 *					"foo": {
 *						"type": "string"
 *					}
 *				},
 *				"cmd_in": []
 *		}
 *
 * 2) The definition of the property of the msg is as follows:
 *
 *		{
 *				"cmd_in": [
 *					{
 *						"name": "demo",
 *						"property": {
 *							"foo": {
 *								"type": "string"
 *							}
 *						},
 *						"required": ["foo"]
 *					}
 *				]
 *		}
 *
 * The type of the property schema is always `axis_schema_object_t` in the
 * schema system. We prefer to parse the `property` schema into a
 * `axis_schema_object_t`, but not a hashmap<name, axis_schema_t>, because some
 * schema definition which is at the same level as `property` (ex: `required`)
 * is used to describe the property.
 */
axis_RUNTIME_PRIVATE_API axis_schema_t *
axis_schemas_parse_schema_object_for_property(axis_value_t *schemas_content);
