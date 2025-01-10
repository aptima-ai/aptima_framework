//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/per_property/rules.h"

#include "include_internal/axis_runtime/msg/cmd_base/cmd_base.h"
#include "include_internal/axis_runtime/msg/field/field.h"
#include "include_internal/axis_runtime/msg/field/field_info.h"
#include "include_internal/axis_runtime/msg/msg.h"
#include "include_internal/axis_runtime/msg_conversion/msg_conversion/per_property/rule.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/json.h"
#include "axis_utils/lib/smart_ptr.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_kv.h"

static axis_msg_conversion_per_property_rules_t *
axis_msg_conversion_per_property_rules_create(void) {
  axis_msg_conversion_per_property_rules_t *self =
      (axis_msg_conversion_per_property_rules_t *)axis_MALLOC(
          sizeof(axis_msg_conversion_per_property_rules_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_list_init(&self->rules);
  self->keep_original = false;

  return self;
}

void axis_msg_conversion_per_property_rules_destroy(
    axis_msg_conversion_per_property_rules_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_list_clear(&self->rules);

  axis_FREE(self);
}

axis_shared_ptr_t *axis_msg_conversion_per_property_rules_convert(
    axis_msg_conversion_per_property_rules_t *self, axis_shared_ptr_t *msg,
    axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");

  axis_shared_ptr_t *new_msg = NULL;

  if (self->keep_original) {
    new_msg = axis_msg_clone(msg, NULL);
  } else {
    // Do _not_ clone 'properties' field.
    axis_list_t excluded_field_ids = axis_LIST_INIT_VAL;
    axis_list_push_back(
        &excluded_field_ids,
        axis_int32_listnode_create(
            axis_msg_fields_info[axis_MSG_FIELD_PROPERTIES].field_id));
    new_msg = axis_msg_clone(msg, &excluded_field_ids);
    axis_list_clear(&excluded_field_ids);
  }

  axis_list_foreach (&self->rules, iter) {
    axis_msg_conversion_per_property_rule_t *property_rule =
        axis_ptr_listnode_get(iter.node);
    axis_ASSERT(property_rule, "Invalid argument.");

    if (!axis_msg_conversion_per_property_rule_convert(property_rule, msg,
                                                      new_msg, err)) {
      axis_shared_ptr_destroy(new_msg);
      new_msg = NULL;
      break;
    }
  }

  return new_msg;
}

axis_shared_ptr_t *axis_result_conversion_per_property_rules_convert(
    axis_msg_conversion_per_property_rules_t *self, axis_shared_ptr_t *msg,
    axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(msg && axis_msg_check_integrity(msg), "Invalid argument.");

  axis_shared_ptr_t *new_msg = NULL;

  if (self->keep_original) {
    new_msg = axis_msg_clone(msg, NULL);
  } else {
    // Do _not_ clone 'properties' field.
    axis_list_t excluded_field_ids = axis_LIST_INIT_VAL;
    axis_list_push_back(
        &excluded_field_ids,
        axis_int32_listnode_create(
            axis_msg_fields_info[axis_MSG_FIELD_PROPERTIES].field_id));
    new_msg = axis_msg_clone(msg, &excluded_field_ids);
    axis_list_clear(&excluded_field_ids);
  }

  // The command ID of the cloned cmd result should be equal to the original
  // cmd result.
  //
  // Note: In the APTIMA runtime, if a command A is cloned from a command B, then
  // the command ID of A & B must be different. However, here is the _ONLY_
  // location where the command ID of the cloned command will be equal to the
  // original command.
  axis_cmd_base_set_cmd_id(new_msg, axis_cmd_base_get_cmd_id(msg));

  // Properties.
  axis_list_foreach (&self->rules, iter) {
    axis_msg_conversion_per_property_rule_t *property_rule =
        axis_ptr_listnode_get(iter.node);
    axis_ASSERT(property_rule, "Invalid argument.");

    if (!axis_msg_conversion_per_property_rule_convert(property_rule, msg,
                                                      new_msg, err)) {
      axis_shared_ptr_destroy(new_msg);
      new_msg = NULL;
      break;
    }
  }

  return new_msg;
}

axis_msg_conversion_per_property_rules_t *
axis_msg_conversion_per_property_rules_from_json(axis_json_t *json,
                                                axis_error_t *err) {
  axis_ASSERT(json, "Invalid argument.");

  axis_msg_conversion_per_property_rules_t *rules =
      axis_msg_conversion_per_property_rules_create();

  size_t index = 0;
  axis_json_t *value = NULL;
  axis_json_array_foreach(json, index, value) {
    axis_msg_conversion_per_property_rule_t *rule =
        axis_msg_conversion_per_property_rule_from_json(value, err);
    if (!rule) {
      axis_msg_conversion_per_property_rules_destroy(rules);
      return NULL;
    }

    axis_list_push_ptr_back(&rules->rules, rule,
                           (axis_ptr_listnode_destroy_func_t)
                               axis_msg_conversion_per_property_rule_destroy);
  }

  return rules;
}

axis_json_t *axis_msg_conversion_per_property_rules_to_json(
    axis_msg_conversion_per_property_rules_t *self, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");

  axis_json_t *rules_json = axis_json_create_array();

  if (axis_list_size(&self->rules) > 0) {
    axis_list_foreach (&self->rules, iter) {
      axis_msg_conversion_per_property_rule_t *rule =
          axis_ptr_listnode_get(iter.node);
      axis_ASSERT(rule, "Invalid argument.");

      axis_json_t *rule_json =
          axis_msg_conversion_per_property_rule_to_json(rule, err);
      if (!rule_json) {
        axis_json_destroy(rules_json);
        return NULL;
      }
      axis_json_array_append_new(rules_json, rule_json);
    }
  }

  return rules_json;
}

axis_msg_conversion_per_property_rules_t *
axis_msg_conversion_per_property_rules_from_value(axis_value_t *value,
                                                 axis_error_t *err) {
  axis_ASSERT(value, "Invalid argument.");

  if (!axis_value_is_array(value)) {
    return NULL;
  }

  axis_msg_conversion_per_property_rules_t *rules =
      axis_msg_conversion_per_property_rules_create();

  axis_list_foreach (&value->content.array, iter) {
    axis_value_t *array_item_value = axis_ptr_listnode_get(iter.node);
    axis_ASSERT(array_item_value && axis_value_check_integrity(array_item_value),
               "Invalid argument.");

    axis_msg_conversion_per_property_rule_t *rule =
        axis_msg_conversion_per_property_rule_from_value(array_item_value, err);
    axis_ASSERT(rule, "Invalid argument.");

    axis_list_push_ptr_back(&rules->rules, rule,
                           (axis_ptr_listnode_destroy_func_t)
                               axis_msg_conversion_per_property_rule_destroy);
  }

  return rules;
}

axis_RUNTIME_PRIVATE_API axis_value_t *
axis_msg_conversion_per_property_rules_to_value(
    axis_msg_conversion_per_property_rules_t *self, axis_error_t *err) {
  axis_ASSERT(self, "Invalid argument.");

  axis_value_t *rules_value = axis_value_create_array_with_move(NULL);

  if (axis_list_size(&self->rules) > 0) {
    axis_list_foreach (&self->rules, iter) {
      axis_msg_conversion_per_property_rule_t *rule =
          axis_ptr_listnode_get(iter.node);
      axis_ASSERT(rule, "Invalid argument.");

      axis_value_t *rule_value =
          axis_msg_conversion_per_property_rule_to_value(rule, err);
      if (!rule_value) {
        axis_value_destroy(rules_value);
        return NULL;
      }

      axis_list_push_ptr_back(
          &rules_value->content.array, rule_value,
          (axis_ptr_listnode_destroy_func_t)axis_value_destroy);
    }
  }

  return rules_value;
}
