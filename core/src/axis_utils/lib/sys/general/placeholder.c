//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_utils/lib/placeholder.h"

#include <stdbool.h>
#include <string.h>

#include "include_internal/axis_utils/common/constant_str.h"
#include "include_internal/axis_utils/macro/memory.h"
#include "include_internal/axis_utils/value/value.h"
#include "axis_runtime/common/errno.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_get.h"

static bool axis_placeholder_check_integrity(axis_placeholder_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_PLACEHOLDER_SIGNATURE) {
    return false;
  }

  return true;
}

bool axis_c_str_is_placeholder(const char *input) {
  axis_ASSERT(input, "Invalid argument.");

  // Check if it starts with ${ and ends with }.
  if (strncmp(input, "${", 2) != 0 || input[strlen(input) - 1] != '}') {
    return false;
  }
  return true;
}

void axis_placeholder_init(axis_placeholder_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_signature_set(&self->signature,
                    (axis_signature_t)axis_PLACEHOLDER_SIGNATURE);
  self->scope = axis_PLACEHOLDER_SCOPE_INVALID;

  axis_value_init_invalid(&self->default_value);
  axis_string_init(&self->variable);
}

axis_placeholder_t *axis_placeholder_create(void) {
  axis_placeholder_t *self = axis_MALLOC(sizeof(axis_placeholder_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_placeholder_init(self);

  return self;
}

void axis_placeholder_deinit(axis_placeholder_t *self) {
  axis_ASSERT(self && axis_placeholder_check_integrity(self),
             "Invalid argument.");

  axis_string_deinit(&self->variable);
  axis_value_deinit(&self->default_value);
}

void axis_placeholder_destroy(axis_placeholder_t *self) {
  axis_ASSERT(self && axis_placeholder_check_integrity(self),
             "Invalid argument.");

  axis_placeholder_deinit(self);

  axis_FREE(self);
}

static axis_PLACEHOLDER_SCOPE axis_placeholder_scope_from_string(
    const char *scope_str) {
  axis_ASSERT(scope_str, "Invalid argument.");

  if (!strcmp(scope_str, axis_STR_ENV)) {
    return axis_PLACEHOLDER_SCOPE_ENV;
  } else {
    axis_ASSERT(0, "Should not happen.");
    return axis_PLACEHOLDER_SCOPE_INVALID;
  }
}

static char *axis_placeholder_escape_characters(const char *input) {
  axis_ASSERT(input, "Invalid argument.");

  // Create a buffer to store the parsed string.
  size_t len = strlen(input);
  char *output = axis_MALLOC(len + 1);
  char *out_ptr = output;
  bool escape = false;

  for (const char *ptr = input; *ptr != '\0'; ptr++) {
    if (*ptr == '\\' && !escape) {
      escape = true;
      continue;
    }

    *out_ptr++ = *ptr;
    escape = false;
  }

  *out_ptr = '\0';
  return output;
}

bool axis_placeholder_parse(axis_placeholder_t *self, const char *input,
                           axis_error_t *err) {
  axis_ASSERT(self && axis_placeholder_check_integrity(self),
             "Invalid argument.");

  if (!axis_c_str_is_placeholder(input)) {
    return false;
  }

  // Remove `${` and `}` and create a temporary, modifiable copy.
  char *content = axis_STRNDUP(input + 2, strlen(input) - 3);

  // Parse the scope part.
  char *scope_end = strchr(content, axis_STR_PLACEHOLDER_SCOPE_DELIMITER);
  if (!scope_end) {
    axis_FREE(content);
    return false;
  }

  *scope_end = '\0';

  self->scope = axis_placeholder_scope_from_string(content);

  // Parse the variable and default_value parts.
  char *variable_start = scope_end + 1;
  char *variable_end = NULL;

  char *escaped_variable = axis_placeholder_escape_characters(variable_start);
  variable_end = strchr(escaped_variable, '|');

  if (variable_end) {
    // Extract variable and default_value.
    *variable_end = '\0';

    axis_string_set_formatted(&self->variable, "%s", escaped_variable);

    if (*(variable_end + 1) == '\0') {
      axis_value_reset_to_string_with_size(&self->default_value, "", 0);
    } else {
      axis_value_reset_to_string_with_size(
          &self->default_value, variable_end + 1, strlen(variable_end + 1));
    }
  } else {
    // Extract only variable.
    axis_string_set_formatted(&self->variable, "%s", variable_start);
  }

  axis_FREE(content);
  axis_FREE(escaped_variable);

  return true;
}

bool axis_placeholder_resolve(axis_placeholder_t *self,
                             axis_value_t *placeholder_value, axis_error_t *err) {
  axis_ASSERT(self && axis_placeholder_check_integrity(self),
             "Invalid argument.");
  axis_ASSERT(placeholder_value != NULL, "Invalid argument.");

  switch (self->scope) {
    case axis_PLACEHOLDER_SCOPE_ENV: {
      const char *variable_name = axis_string_get_raw_str(&self->variable);
      const char *env_value = getenv(variable_name);

      if (env_value != NULL) {
        // Environment variable found, set the resolved value
        axis_value_reset_to_string_with_size(placeholder_value, env_value,
                                            strlen(env_value));
      } else {
        // Environment variable not found, use default value.
        if (!axis_value_is_valid(&self->default_value)) {
          // If no default value is provided, use 'null' value.
          axis_LOGE(
              "Environment variable %s is not found, neither default value is "
              "provided.",
              variable_name);

          // NOLINTNEXTLINE(concurrency-mt-unsafe)
          exit(EXIT_FAILURE);

          axis_value_reset_to_null(placeholder_value);
        } else {
          const char *default_value =
              axis_value_peek_raw_str(&self->default_value, err);

          axis_LOGI(
              "Environment variable %s is not found, using default value %s.",
              variable_name, default_value);

          axis_value_reset_to_string_with_size(placeholder_value, default_value,
                                              strlen(default_value));
        }
      }
      break;
    }

    default:
      if (err) {
        axis_error_set(err, axis_ERRNO_GENERIC,
                      "Unsupported placeholder scope: %d", self->scope);
      }
      axis_ASSERT(0, "Should not happen.");
      return false;
  }

  return true;
}
