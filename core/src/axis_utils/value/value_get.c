//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include <float.h>
#include <stdint.h>
#include <stdlib.h>

#include "axis_runtime/common/errno.h"
#include "axis_utils/container/list.h"
#include "axis_utils/container/list_node.h"
#include "axis_utils/lib/error.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/value/type.h"
#include "axis_utils/value/type_operation.h"
#include "axis_utils/value/value.h"
#include "axis_utils/value/value_is.h"
#include "axis_utils/value/value_kv.h"

int8_t axis_value_get_int8(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_valid(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Invalid value type.");
    }
    return 0;
  }

  switch (self->type) {
    case axis_TYPE_INT8:
      return self->content.int8;
    case axis_TYPE_INT16:
      if (self->content.int16 >= -INT8_MAX && self->content.int16 <= INT8_MAX) {
        return (int8_t)self->content.int16;
      }
      break;
    case axis_TYPE_INT32:
      if (self->content.int32 >= -INT8_MAX && self->content.int32 <= INT8_MAX) {
        return (int8_t)self->content.int32;
      }
      break;
    case axis_TYPE_INT64:
      if (self->content.int64 >= -INT8_MAX && self->content.int64 <= INT8_MAX) {
        return (int8_t)self->content.int64;
      }
      break;
    case axis_TYPE_UINT8:
      if (self->content.uint8 <= INT8_MAX) {
        return (int8_t)self->content.uint8;
      }
      break;
    case axis_TYPE_UINT16:
      if (self->content.uint16 <= INT8_MAX) {
        return (int8_t)self->content.uint16;
      }
      break;
    case axis_TYPE_UINT32:
      if (self->content.uint32 <= INT8_MAX) {
        return (int8_t)self->content.uint32;
      }
      break;
    case axis_TYPE_UINT64:
      if (self->content.uint64 <= INT8_MAX) {
        return (int8_t)self->content.uint64;
      }
      break;
    default:
      break;
  }

  if (err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The conversion from %s to uint32 is unfit.",
                  axis_type_to_string(self->type));
  }
  return 0;
}

int16_t axis_value_get_int16(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_valid(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Invalid value type.");
    }
    return 0;
  }

  switch (self->type) {
    case axis_TYPE_INT8:
      return self->content.int8;
    case axis_TYPE_INT16:
      return self->content.int16;
    case axis_TYPE_INT32:
      if (self->content.int32 >= -INT16_MAX &&
          self->content.int32 <= INT16_MAX) {
        return (int16_t)self->content.int32;
      }
      break;
    case axis_TYPE_INT64:
      if (self->content.int64 >= -INT16_MAX &&
          self->content.int64 <= INT16_MAX) {
        return (int16_t)self->content.int64;
      }
      break;
    case axis_TYPE_UINT8:
      return (int16_t)self->content.uint8;
    case axis_TYPE_UINT16:
      if (self->content.uint16 <= INT16_MAX) {
        return (int16_t)self->content.uint16;
      }
      break;
    case axis_TYPE_UINT32:
      if (self->content.uint32 <= INT16_MAX) {
        return (int16_t)self->content.uint32;
      }
      break;
    case axis_TYPE_UINT64:
      if (self->content.uint64 <= INT16_MAX) {
        return (int16_t)self->content.uint64;
      }
      break;
    default:
      break;
  }

  if (err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The conversion from %s to uint32 is unfit.",
                  axis_type_to_string(self->type));
  }
  return 0;
}

int32_t axis_value_get_int32(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_valid(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Invalid value type.");
    }
    return 0;
  }

  switch (self->type) {
    case axis_TYPE_INT8:
      return self->content.int8;
    case axis_TYPE_INT16:
      return self->content.int16;
    case axis_TYPE_INT32:
      return self->content.int32;
    case axis_TYPE_INT64:
      if (self->content.int64 >= -INT32_MAX &&
          self->content.int64 <= INT32_MAX) {
        return (int32_t)self->content.int64;
      }
      break;
    case axis_TYPE_UINT8:
      return self->content.uint8;
    case axis_TYPE_UINT16:
      return self->content.uint16;
    case axis_TYPE_UINT32:
      if (self->content.uint32 <= INT32_MAX) {
        return (int32_t)self->content.uint32;
      }
      break;
    case axis_TYPE_UINT64:
      if (self->content.uint64 <= INT32_MAX) {
        return (int32_t)self->content.uint64;
      }
      break;
    default:
      break;
  }

  if (err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The conversion from %s to uint32 is unfit.",
                  axis_type_to_string(self->type));
  }
  return 0;
}

int64_t axis_value_get_int64(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_valid(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Invalid value type.");
    }
    return 0;
  }

  switch (self->type) {
    case axis_TYPE_INT8:
      return self->content.int8;
    case axis_TYPE_INT16:
      return self->content.int16;
    case axis_TYPE_INT32:
      return self->content.int32;
    case axis_TYPE_INT64:
      return self->content.int64;
    case axis_TYPE_UINT8:
      return self->content.uint8;
    case axis_TYPE_UINT16:
      return self->content.uint16;
    case axis_TYPE_UINT32:
      return self->content.uint32;
    case axis_TYPE_UINT64:
      if (self->content.uint64 <= INT64_MAX) {
        return (int64_t)self->content.uint64;
      }
      break;
    default:
      break;
  }

  if (err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The conversion from %s to uint32 is unfit.",
                  axis_type_to_string(self->type));
  }
  return 0;
}

uint8_t axis_value_get_uint8(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_valid(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Invalid value type.");
    }
    return 0;
  }

  switch (self->type) {
    case axis_TYPE_UINT8:
      return self->content.uint8;
    case axis_TYPE_UINT16:
      if (self->content.uint64 <= UINT8_MAX) {
        return (uint8_t)self->content.uint16;
      }
      break;
    case axis_TYPE_UINT32:
      if (self->content.uint64 <= UINT8_MAX) {
        return (uint8_t)self->content.uint32;
      }
      break;
    case axis_TYPE_UINT64:
      if (self->content.uint64 <= UINT8_MAX) {
        return (uint8_t)self->content.uint64;
      }
      break;
    case axis_TYPE_INT8:
      if (self->content.int8 >= 0) {
        return (uint8_t)self->content.int8;
      }
      break;
    case axis_TYPE_INT16:
      if (self->content.int16 >= 0 && self->content.int16 <= UINT8_MAX) {
        return (uint8_t)self->content.int16;
      }
      break;
    case axis_TYPE_INT32:
      if (self->content.int32 >= 0 && self->content.int32 <= UINT8_MAX) {
        return (uint8_t)self->content.int32;
      }
      break;
    case axis_TYPE_INT64:
      if (self->content.int64 >= 0 && self->content.int64 <= UINT8_MAX) {
        return (uint8_t)self->content.int64;
      }
      break;
    default:
      break;
  }

  if (err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The conversion from %s to uint32 is unfit.",
                  axis_type_to_string(self->type));
  }
  return 0;
}

uint16_t axis_value_get_uint16(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_valid(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Invalid value type.");
    }
    return 0;
  }

  switch (self->type) {
    case axis_TYPE_UINT8:
      return self->content.uint8;
    case axis_TYPE_UINT16:
      return self->content.uint16;
    case axis_TYPE_UINT32:
      if (self->content.uint64 <= UINT16_MAX) {
        return (uint16_t)self->content.uint32;
      }
      break;
    case axis_TYPE_UINT64:
      if (self->content.uint64 <= UINT16_MAX) {
        return (uint16_t)self->content.uint64;
      }
      break;
    case axis_TYPE_INT8:
      if (self->content.int8 >= 0) {
        return (uint16_t)self->content.int8;
      }
      break;
    case axis_TYPE_INT16:
      if (self->content.int16 >= 0) {
        return (uint16_t)self->content.int16;
      }
      break;
    case axis_TYPE_INT32:
      if (self->content.int32 >= 0 && self->content.int64 <= UINT16_MAX) {
        return (uint16_t)self->content.int32;
      }
      break;
    case axis_TYPE_INT64:
      if (self->content.int64 >= 0 && self->content.int64 <= UINT16_MAX) {
        return (uint16_t)self->content.int64;
      }
      break;
    default:
      break;
  }

  if (err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The conversion from %s to uint32 is unfit.",
                  axis_type_to_string(self->type));
  }
  return 0;
}

uint32_t axis_value_get_uint32(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_valid(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Invalid value type.");
    }
    return 0;
  }

  switch (self->type) {
    case axis_TYPE_UINT8:
      return self->content.uint8;
    case axis_TYPE_UINT16:
      return self->content.uint16;
    case axis_TYPE_UINT32:
      return self->content.uint32;
    case axis_TYPE_UINT64:
      if (self->content.uint64 <= UINT32_MAX) {
        return (uint32_t)self->content.uint64;
      }
      break;
    case axis_TYPE_INT8:
      if (self->content.int8 >= 0) {
        return (uint32_t)self->content.int8;
      }
      break;
    case axis_TYPE_INT16:
      if (self->content.int16 >= 0) {
        return (uint32_t)self->content.int16;
      }
      break;
    case axis_TYPE_INT32:
      if (self->content.int32 >= 0) {
        return (uint32_t)self->content.int32;
      }
      break;
    case axis_TYPE_INT64:
      if (self->content.int64 >= 0 && self->content.int64 <= UINT32_MAX) {
        return (uint32_t)self->content.int64;
      }
      break;
    default:
      break;
  }

  if (err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The conversion from %s to uint32 is unfit.",
                  axis_type_to_string(self->type));
  }
  return 0;
}

uint64_t axis_value_get_uint64(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_valid(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Invalid value type.");
    }
    return 0;
  }

  switch (self->type) {
    case axis_TYPE_UINT8:
      return self->content.uint8;
    case axis_TYPE_UINT16:
      return self->content.uint16;
    case axis_TYPE_UINT32:
      return self->content.uint32;
    case axis_TYPE_UINT64:
      return self->content.uint64;
    case axis_TYPE_INT8:
      if (self->content.int8 >= 0) {
        return (uint64_t)self->content.int8;
      }
      break;
    case axis_TYPE_INT16:
      if (self->content.int16 >= 0) {
        return (uint64_t)self->content.int16;
      }
      break;
    case axis_TYPE_INT32:
      if (self->content.int32 >= 0) {
        return (uint64_t)self->content.int32;
      }
      break;
    case axis_TYPE_INT64:
      if (self->content.int64 >= 0) {
        return (uint64_t)self->content.int64;
      }
      break;
    default:
      break;
  }

  if (err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The conversion from %s to uint32 is unfit.",
                  axis_type_to_string(self->type));
  }
  return 0;
}

float axis_value_get_float32(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_valid(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Invalid value type.");
    }
    return 0.0F;
  }

  switch (self->type) {
    case axis_TYPE_FLOAT32:
      return self->content.float32;
    case axis_TYPE_FLOAT64:
      if (self->content.float64 >= -FLT_MAX &&
          self->content.float64 <= FLT_MAX) {
        return (float)self->content.float64;
      }
      break;
    default:
      return 0.0F;
  }

  if (err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The conversion from %s to float32 is unfit.",
                  axis_type_to_string(self->type));
  }
  return 0.0F;
}

double axis_value_get_float64(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_valid(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Invalid value type.");
    }
    return 0.0F;
  }

  switch (self->type) {
    case axis_TYPE_FLOAT32:
      return self->content.float32;
    case axis_TYPE_FLOAT64:
      return self->content.float64;
    default:
      break;
  }

  if (err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The conversion from %s to float32 is unfit.",
                  axis_type_to_string(self->type));
  }
  return 0.0;
}

bool axis_value_get_bool(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_valid(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC, "Invalid value type.");
    }
    return false;
  }

  switch (self->type) {
    case axis_TYPE_BOOL:
      return self->content.boolean;
    default:
      break;
  }

  if (err) {
    axis_error_set(err, axis_ERRNO_GENERIC,
                  "The conversion from %s to uint32 is unfit.",
                  axis_type_to_string(self->type));
  }
  return false;
}

void *axis_value_get_ptr(axis_value_t *self, axis_error_t *err) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");

  if (axis_value_is_ptr(self)) {
    return self->content.ptr;
  } else {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC,
                    "Not pointer value, actual type: %s",
                    axis_type_to_string(self->type));
    }
    return NULL;
  }
}

axis_buf_t *axis_value_peek_buf(axis_value_t *self) {
  if (!self) {
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (axis_value_is_buf(self)) {
    return &self->content.buf;
  }
  return NULL;
}

axis_list_t *axis_value_peek_array(axis_value_t *self) {
  if (!self) {
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (axis_value_is_array(self)) {
    return &self->content.array;
  }
  return NULL;
}

axis_list_t *axis_value_peek_object(axis_value_t *self) {
  if (!self) {
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (axis_value_is_object(self)) {
    return &self->content.object;
  }
  return NULL;
}

axis_string_t *axis_value_peek_string(axis_value_t *self) {
  if (!self) {
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (axis_value_is_string(self)) {
    return &self->content.string;
  } else {
    return NULL;
  }
}

const char *axis_value_peek_raw_str(axis_value_t *self, axis_error_t *err) {
  if (!self) {
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (axis_value_is_string(self)) {
    return axis_string_get_raw_str(&self->content.string);
  }
  return NULL;
}

axis_TYPE axis_value_get_type(axis_value_t *self) {
  axis_ASSERT(self && axis_value_check_integrity(self), "Invalid argument.");
  return self->type;
}

axis_value_t *axis_value_array_peek(axis_value_t *self, size_t index,
                                  axis_error_t *err) {
  if (!self) {
    return NULL;
  }

  axis_ASSERT(axis_value_check_integrity(self), "Invalid argument.");

  if (!axis_value_is_array(self)) {
    if (err) {
      axis_error_set(err, axis_ERRNO_GENERIC,
                    "The conversion from %s to array is unfit.",
                    axis_type_to_string(self->type));
    }
    return NULL;
  }

  if (index >= axis_list_size(&self->content.array)) {
    return NULL;
  }

  axis_value_array_foreach(self, iter) {
    if (iter.index == index) {
      return axis_ptr_listnode_get(iter.node);
    }
  }

  axis_ASSERT(0, "Should not happen.");
  return NULL;
}
