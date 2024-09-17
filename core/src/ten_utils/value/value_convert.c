//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_utils/value/value_convert.h"

#include <float.h>
#include <stdint.h>

#include "ten_utils/macro/check.h"
#include "include_internal/ten_utils/value/value_can_convert.h"
#include "ten_runtime/common/errno.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/value/type.h"
#include "ten_utils/value/type_operation.h"
#include "ten_utils/value/value.h"

bool ten_value_convert_to_int8(ten_value_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  switch (self->type) {
    case TEN_TYPE_INT8:
      return true;

    case TEN_TYPE_INT16: {
      int16_t content = self->content.int16;
      if (content < INT8_MIN || content > INT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int8.");
        }
        return false;
      }

      return ten_value_init_int8(self, (int8_t)content);
    }

    case TEN_TYPE_INT32: {
      int32_t content = self->content.int32;
      if (content < INT8_MIN || content > INT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int8.");
        }
        return false;
      }

      return ten_value_init_int8(self, (int8_t)content);
    }

    case TEN_TYPE_INT64: {
      int64_t content = self->content.int64;
      if (content < INT8_MIN || content > INT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int8.");
        }
        return false;
      }

      return ten_value_init_int8(self, (int8_t)content);
    }

    case TEN_TYPE_UINT8: {
      uint8_t content = self->content.uint8;
      if (content > INT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int8.");
        }
        return false;
      }

      return ten_value_init_int8(self, (int8_t)content);
    }

    case TEN_TYPE_UINT16: {
      uint16_t content = self->content.uint16;
      if (content > INT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int8.");
        }
        return false;
      }

      return ten_value_init_int8(self, (int8_t)content);
    }

    case TEN_TYPE_UINT32: {
      uint32_t content = self->content.uint32;
      if (content > INT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int8.");
        }
        return false;
      }

      return ten_value_init_int8(self, (int8_t)content);
    }

    case TEN_TYPE_UINT64: {
      uint64_t content = self->content.uint64;
      if (content > INT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int8.");
        }
        return false;
      }

      return ten_value_init_int8(self, (int8_t)content);
    }

    default:
      if (err) {
        ten_error_set(err, TEN_ERRNO_GENERIC,
                      "Unsupported conversion from `%s` to int8.",
                      ten_type_to_string(self->type));
      }
      return false;
  }
}

bool ten_value_convert_to_int16(ten_value_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  switch (self->type) {
    case TEN_TYPE_INT8: {
      return ten_value_init_int16(self, (int16_t)self->content.int8);
    }

    case TEN_TYPE_INT16: {
      return true;
    }

    case TEN_TYPE_INT32: {
      int32_t content = self->content.int32;
      if (content < INT16_MIN || content > INT16_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int16.");
        }
        return false;
      }

      return ten_value_init_int16(self, (int16_t)content);
    }

    case TEN_TYPE_INT64: {
      int64_t content = self->content.int64;
      if (content < INT16_MIN || content > INT16_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int16.");
        }
        return false;
      }

      return ten_value_init_int16(self, (int16_t)content);
    }

    case TEN_TYPE_UINT8: {
      return ten_value_init_int16(self, (int16_t)self->content.uint8);
    }

    case TEN_TYPE_UINT16: {
      uint16_t content = self->content.uint16;
      if (content > INT16_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int16.");
        }
        return false;
      }

      return ten_value_init_int16(self, (int16_t)content);
    }

    case TEN_TYPE_UINT32: {
      uint32_t content = self->content.uint32;
      if (content > INT16_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int16.");
        }
        return false;
      }

      return ten_value_init_int16(self, (int16_t)content);
    }

    case TEN_TYPE_UINT64: {
      uint64_t content = self->content.uint64;
      if (content > INT16_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int16.");
        }
        return false;
      }

      return ten_value_init_int16(self, (int16_t)content);
    }

    default:
      if (err) {
        ten_error_set(err, TEN_ERRNO_GENERIC,
                      "Unsupported conversion from `%s` to int16.",
                      ten_type_to_string(self->type));
      }
      return false;
  }
}

bool ten_value_convert_to_int32(ten_value_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  switch (self->type) {
    case TEN_TYPE_INT8: {
      return ten_value_init_int32(self, (int32_t)self->content.int8);
    }

    case TEN_TYPE_INT16: {
      return ten_value_init_int32(self, (int32_t)self->content.int16);
    }

    case TEN_TYPE_INT32: {
      return true;
    }

    case TEN_TYPE_INT64: {
      int64_t content = self->content.int64;
      if (content < INT32_MIN || content > INT32_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int32.");
        }
        return false;
      }

      return ten_value_init_int32(self, (int32_t)content);
    }

    case TEN_TYPE_UINT8: {
      return ten_value_init_int32(self, (int32_t)self->content.uint8);
    }

    case TEN_TYPE_UINT16: {
      return ten_value_init_int32(self, (int32_t)self->content.uint16);
    }

    case TEN_TYPE_UINT32: {
      uint32_t content = self->content.uint32;
      if (content > INT32_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int32.");
        }
        return false;
      }

      return ten_value_init_int32(self, (int32_t)content);
    }

    case TEN_TYPE_UINT64: {
      uint64_t content = self->content.uint64;
      if (content > INT32_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int32.");
        }
        return false;
      }

      return ten_value_init_int32(self, (int32_t)content);
    }

    default:
      if (err) {
        ten_error_set(err, TEN_ERRNO_GENERIC,
                      "Unsupported conversion from `%s` to int32.",
                      ten_type_to_string(self->type));
      }
      return false;
  }
}

bool ten_value_convert_to_int64(ten_value_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  switch (self->type) {
    case TEN_TYPE_INT8: {
      return ten_value_init_int64(self, (int64_t)self->content.int8);
    }

    case TEN_TYPE_INT16: {
      return ten_value_init_int64(self, (int64_t)self->content.int16);
    }

    case TEN_TYPE_INT32: {
      return ten_value_init_int64(self, (int64_t)self->content.int32);
    }

    case TEN_TYPE_INT64: {
      return true;
    }

    case TEN_TYPE_UINT8: {
      return ten_value_init_int64(self, (int64_t)self->content.uint8);
    }

    case TEN_TYPE_UINT16: {
      return ten_value_init_int64(self, (int64_t)self->content.uint16);
    }

    case TEN_TYPE_UINT32: {
      return ten_value_init_int64(self, (int64_t)self->content.uint32);
    }

    case TEN_TYPE_UINT64: {
      uint64_t content = self->content.uint64;
      if (content > INT64_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of int64.");
        }
        return false;
      }

      return ten_value_init_int64(self, (int64_t)content);
    }

    default:
      if (err) {
        ten_error_set(err, TEN_ERRNO_GENERIC,
                      "Unsupported conversion from `%s` to int64.",
                      ten_type_to_string(self->type));
      }
      return false;
  }
}

bool ten_value_convert_to_uint8(ten_value_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  switch (self->type) {
    case TEN_TYPE_INT8: {
      int8_t content = self->content.int8;
      if (content < 0) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint8.");
        }
        return false;
      }

      return ten_value_init_uint8(self, (uint8_t)content);
    }

    case TEN_TYPE_INT16: {
      int16_t content = self->content.int16;
      if (content < 0 || content > UINT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint8.");
        }
        return false;
      }

      return ten_value_init_uint8(self, (uint8_t)content);
    }

    case TEN_TYPE_INT32: {
      int32_t content = self->content.int32;
      if (content < 0 || content > UINT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint8.");
        }
        return false;
      }

      return ten_value_init_uint8(self, (uint8_t)content);
    }

    case TEN_TYPE_INT64: {
      int64_t content = self->content.int64;
      if (content < 0 || content > UINT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint8.");
        }
        return false;
      }

      return ten_value_init_uint8(self, (uint8_t)content);
    }

    case TEN_TYPE_UINT8: {
      return true;
    }

    case TEN_TYPE_UINT16: {
      uint16_t content = self->content.uint16;
      if (content > UINT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint8.");
        }
        return false;
      }

      return ten_value_init_uint8(self, (uint8_t)content);
    }

    case TEN_TYPE_UINT32: {
      uint32_t content = self->content.uint32;
      if (content > UINT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint8.");
        }
        return false;
      }

      return ten_value_init_uint8(self, (uint8_t)content);
    }

    case TEN_TYPE_UINT64: {
      uint64_t content = self->content.uint64;
      if (content > UINT8_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint8.");
        }
        return false;
      }

      return ten_value_init_uint8(self, (uint8_t)content);
    }

    default:
      if (err) {
        ten_error_set(err, TEN_ERRNO_GENERIC,
                      "Unsupported conversion from `%s` to uint8.",
                      ten_type_to_string(self->type));
      }
      return false;
  }
}

bool ten_value_convert_to_uint16(ten_value_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  switch (self->type) {
    case TEN_TYPE_INT8: {
      int8_t content = self->content.int8;
      if (content < 0) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint16.");
        }
        return false;
      }

      return ten_value_init_uint16(self, (uint16_t)content);
    }

    case TEN_TYPE_INT16: {
      int16_t content = self->content.int16;
      if (content < 0) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint16.");
        }
        return false;
      }

      return ten_value_init_uint16(self, (uint16_t)content);
    }

    case TEN_TYPE_INT32: {
      int32_t content = self->content.int32;
      if (content < 0 || content > UINT16_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint16.");
        }
        return false;
      }

      return ten_value_init_uint16(self, (uint16_t)content);
    }

    case TEN_TYPE_INT64: {
      int64_t content = self->content.int64;
      if (content < 0 || content > UINT16_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint16.");
        }
        return false;
      }

      return ten_value_init_uint16(self, (uint16_t)content);
    }

    case TEN_TYPE_UINT8: {
      return ten_value_init_uint16(self, (uint16_t)self->content.uint8);
    }

    case TEN_TYPE_UINT16: {
      return true;
    }

    case TEN_TYPE_UINT32: {
      uint32_t content = self->content.uint32;
      if (content > UINT16_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint16.");
        }
        return false;
      }

      return ten_value_init_uint16(self, (uint16_t)content);
    }

    case TEN_TYPE_UINT64: {
      uint64_t content = self->content.uint64;
      if (content > UINT16_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint16.");
        }
        return false;
      }

      return ten_value_init_uint16(self, (uint16_t)content);
    }

    default:
      if (err) {
        ten_error_set(err, TEN_ERRNO_GENERIC,
                      "Unsupported conversion from `%s` to uint16.",
                      ten_type_to_string(self->type));
      }
      return false;
  }
}

bool ten_value_convert_to_uint32(ten_value_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  switch (self->type) {
    case TEN_TYPE_INT8: {
      int8_t content = self->content.int8;
      if (content < 0) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint32.");
        }
        return false;
      }

      return ten_value_init_uint32(self, (uint32_t)content);
    }

    case TEN_TYPE_INT16: {
      int16_t content = self->content.int16;
      if (content < 0) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint32.");
        }
        return false;
      }

      return ten_value_init_uint32(self, (uint32_t)content);
    }

    case TEN_TYPE_INT32: {
      int32_t content = self->content.int32;
      if (content < 0) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint32.");
        }
        return false;
      }

      return ten_value_init_uint32(self, (uint32_t)content);
    }

    case TEN_TYPE_INT64: {
      int64_t content = self->content.int64;
      if (content < 0 || content > UINT32_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint32.");
        }
        return false;
      }

      return ten_value_init_uint32(self, (uint32_t)content);
    }

    case TEN_TYPE_UINT8: {
      return ten_value_init_uint32(self, (uint32_t)self->content.uint8);
    }

    case TEN_TYPE_UINT16: {
      return ten_value_init_uint32(self, (uint32_t)self->content.uint16);
    }

    case TEN_TYPE_UINT32: {
      return true;
    }

    case TEN_TYPE_UINT64: {
      uint64_t content = self->content.uint64;
      if (content > UINT32_MAX) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint32.");
        }
        return false;
      }

      return ten_value_init_uint32(self, (uint32_t)content);
    }

    default:
      if (err) {
        ten_error_set(err, TEN_ERRNO_GENERIC,
                      "Unsupported conversion from `%s` to uint32.",
                      ten_type_to_string(self->type));
      }
      return false;
  }
}

bool ten_value_convert_to_uint64(ten_value_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  switch (self->type) {
    case TEN_TYPE_INT8: {
      int8_t content = self->content.int8;
      if (content < 0) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint64.");
        }
        return false;
      }

      return ten_value_init_uint64(self, (uint64_t)content);
    }

    case TEN_TYPE_INT16: {
      int16_t content = self->content.int16;
      if (content < 0) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint64.");
        }
        return false;
      }

      return ten_value_init_uint64(self, (uint64_t)content);
    }

    case TEN_TYPE_INT32: {
      int32_t content = self->content.int32;
      if (content < 0) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint64.");
        }
        return false;
      }

      return ten_value_init_uint64(self, (uint64_t)content);
    }

    case TEN_TYPE_INT64: {
      int64_t content = self->content.int64;
      if (content < 0) {
        if (err) {
          ten_error_set(err, TEN_ERRNO_GENERIC, "Out of range of uint64.");
        }
        return false;
      }

      return ten_value_init_uint64(self, (uint64_t)content);
    }

    case TEN_TYPE_UINT8: {
      return ten_value_init_uint64(self, (uint64_t)self->content.uint8);
    }

    case TEN_TYPE_UINT16: {
      return ten_value_init_uint64(self, (uint64_t)self->content.uint16);
    }

    case TEN_TYPE_UINT32: {
      return ten_value_init_uint64(self, (uint64_t)self->content.uint32);
    }

    case TEN_TYPE_UINT64: {
      return true;
    }

    default:
      if (err) {
        ten_error_set(err, TEN_ERRNO_GENERIC,
                      "Unsupported conversion from `%s` to uint64.",
                      ten_type_to_string(self->type));
      }
      return false;
  }
}

bool ten_value_convert_to_float32(ten_value_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  switch (self->type) {
    case TEN_TYPE_INT8:
      ten_value_init_float32(self, (float)self->content.int8);
      return true;
    case TEN_TYPE_INT16:
      ten_value_init_float32(self, (float)self->content.int16);
      return true;
    case TEN_TYPE_INT32:
      if (can_convert_int32_to_float32(self->content.int32)) {
        ten_value_init_float32(self, (float)self->content.int32);
        return true;
      }
      break;
    case TEN_TYPE_INT64:
      if (can_convert_int64_to_float32(self->content.int64)) {
        ten_value_init_float32(self, (float)self->content.int64);
        return true;
      }
      break;

    case TEN_TYPE_UINT8:
      ten_value_init_float32(self, (float)self->content.uint8);
      return true;
    case TEN_TYPE_UINT16:
      ten_value_init_float32(self, (float)self->content.uint16);
      return true;
    case TEN_TYPE_UINT32:
      if (can_convert_uint32_to_float32(self->content.uint32)) {
        ten_value_init_float32(self, (float)self->content.uint32);
        return true;
      }
      break;
    case TEN_TYPE_UINT64:
      if (can_convert_uint64_to_float32(self->content.uint64)) {
        ten_value_init_float32(self, (float)self->content.uint64);
        return true;
      }
      break;

    case TEN_TYPE_FLOAT32:
      return true;

    case TEN_TYPE_FLOAT64: {
      double content = self->content.float64;
      if (content >= -FLT_MAX && content <= FLT_MAX) {
        return ten_value_init_float32(self, (float)content);
      }
      break;
    }

    default:
      break;
  }

  if (err) {
    ten_error_set(err, TEN_ERRNO_GENERIC,
                  "Unsupported conversion from `%s` to float32.",
                  ten_type_to_string(self->type));
  }
  return false;
}

bool ten_value_convert_to_float64(ten_value_t *self, ten_error_t *err) {
  TEN_ASSERT(self && ten_value_check_integrity(self), "Invalid argument.");
  TEN_ASSERT(err && ten_error_check_integrity(err), "Invalid argument.");

  switch (self->type) {
    case TEN_TYPE_INT8:
      ten_value_init_float64(self, (float)self->content.int8);
      return true;
    case TEN_TYPE_INT16:
      ten_value_init_float64(self, (float)self->content.int16);
      return true;
    case TEN_TYPE_INT32:
      ten_value_init_float64(self, (float)self->content.int32);
      return true;
    case TEN_TYPE_INT64:
      if (can_convert_int64_to_float64(self->content.int64)) {
        ten_value_init_float64(self, (float)self->content.int64);
        return true;
      }
      break;

    case TEN_TYPE_UINT8:
      ten_value_init_float64(self, (float)self->content.uint8);
      return true;
    case TEN_TYPE_UINT16:
      ten_value_init_float64(self, (float)self->content.uint16);
      return true;
    case TEN_TYPE_UINT32:
      ten_value_init_float64(self, (float)self->content.uint32);
      return true;
    case TEN_TYPE_UINT64:
      if (can_convert_uint64_to_float64(self->content.uint64)) {
        ten_value_init_float64(self, (float)self->content.uint64);
        return true;
      }
      break;

    case TEN_TYPE_FLOAT32:
      return ten_value_init_float64(self, (double)self->content.float32);

    case TEN_TYPE_FLOAT64:
      return true;

    default:
      break;
  }

  if (err) {
    ten_error_set(err, TEN_ERRNO_GENERIC,
                  "Unsupported conversion from `%s` to float64.",
                  ten_type_to_string(self->type));
  }
  return false;
}
