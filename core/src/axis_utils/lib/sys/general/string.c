//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/string.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "axis_utils/container/list.h"
#include "axis_utils/lib/alloc.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/memory.h"

#if defined(_WIN32) || defined(_WIN64)
#define strtok_r strtok_s
#endif

axis_string_t *axis_string_create(void) {
  axis_string_t *self = (axis_string_t *)axis_MALLOC(sizeof(axis_string_t));
  axis_ASSERT(self, "Failed to allocate memory.");

  axis_string_init(self);
  return self;
}

axis_string_t *axis_string_create_from_c_str(const char *str, size_t size) {
  axis_ASSERT(str, "Invalid argument.");

  axis_string_t *result = axis_string_create();
  axis_string_set_from_c_str(result, str, size);

  return result;
}

enum { MAX_RETRIES = 10 };

void axis_string_append_from_va_list(axis_string_t *self, const char *fmt,
                                    va_list ap) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");

  size_t retry_count = 0;

  va_list cp;
  va_copy(cp, ap);

  for (;;) {
    va_list temp;
    va_copy(temp, cp);

    int n = vsnprintf(&self->buf[self->first_unused_idx],
                      self->buf_size - self->first_unused_idx, fmt, temp);

    va_end(temp);

    if ((n > -1) && ((size_t)n < (self->buf_size - self->first_unused_idx))) {
      self->first_unused_idx += n;
      va_end(cp);
      return;
    }

    // Else try again with more space.
    if (n > -1) {
      axis_string_reserve(self, n + 1);  // Exact
    } else {
      axis_string_reserve(self, self->buf_size * 2);  // 2x
    }

    if (++retry_count > MAX_RETRIES) {
      va_end(cp);

      axis_ASSERT(0, "Should not happen");
      return;
    }
  }
}

axis_string_t *axis_string_create_formatted(const char *fmt, ...) {
  axis_string_t *self = axis_string_create();
  va_list ap;
  va_start(ap, fmt);
  axis_string_append_from_va_list(self, fmt, ap);
  va_end(ap);
  return self;
}

axis_string_t *axis_string_create_from_va_list(const char *fmt, va_list ap) {
  axis_string_t *self = axis_string_create();
  axis_string_append_from_va_list(self, fmt, ap);
  return self;
}

axis_string_t *axis_string_clone(const axis_string_t *other) {
  axis_ASSERT(other && axis_string_check_integrity(other), "Invalid argument.");

  return axis_string_create_formatted("%s", axis_string_get_raw_str(other));
}

void axis_string_destroy(axis_string_t *self) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");
  axis_string_deinit(self);
  axis_FREE(self);
}

void axis_string_init(axis_string_t *self) {
  axis_ASSERT(self, "Invalid argument.");
  // axis_ASSERT(axis_signature_get(&self->signature) != axis_STRING_SIGNATURE,
  //            "Should not happen.");

  axis_signature_set(&self->signature, (axis_signature_t)axis_STRING_SIGNATURE);

  self->buf = self->pre_buf;
  self->buf_size = axis_STRING_PRE_BUF_SIZE;
  self->first_unused_idx = 0;
  self->buf[0] = 0;
}

void axis_string_init_from_va_list(axis_string_t *self, const char *fmt,
                                  va_list ap) {
  axis_ASSERT(self, "Invalid argument.");

  axis_string_init(self);
  axis_string_append_from_va_list(self, fmt, ap);
}

void axis_string_init_from_string(axis_string_t *self, axis_string_t *other) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(other && axis_string_check_integrity(other) &&
                 axis_string_get_raw_str(other),
             "Invalid argument.");

  axis_string_init_formatted(self, "%s", axis_string_get_raw_str(other));
}

void axis_string_copy(axis_string_t *self, axis_string_t *other) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(other && axis_string_check_integrity(other) &&
                 axis_string_get_raw_str(other),
             "Invalid argument.");

  axis_string_set_formatted(self, "%s", axis_string_get_raw_str(other));
}

void axis_string_init_formatted(axis_string_t *self, const char *fmt, ...) {
  axis_ASSERT(self, "Invalid argument.");

  va_list ap;
  va_start(ap, fmt);
  axis_string_init_from_va_list(self, fmt, ap);
  va_end(ap);
}

void axis_string_init_from_c_str(axis_string_t *self, const char *str,
                                size_t size) {
  axis_ASSERT(self && str, "Invalid argument.");

  axis_string_init(self);
  axis_string_set_formatted(self, "%.*s", size, str);
}

void axis_string_set_from_c_str(axis_string_t *self, const char *str,
                               size_t size) {
  axis_ASSERT(self && axis_string_check_integrity(self) && str,
             "Invalid argument.");
  axis_ASSERT(size, "Invalid argument.");

  axis_string_set_formatted(self, "%.*s", size, str);
}

void axis_string_set_formatted(axis_string_t *self, const char *fmt, ...) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");

  axis_string_clear(self);

  va_list ap;
  va_start(ap, fmt);
  axis_string_append_from_va_list(self, fmt, ap);
  va_end(ap);
}

void axis_string_prepend_from_va_list(axis_string_t *self, const char *fmt,
                                     va_list ap) {
  axis_ASSERT(self && axis_string_check_integrity(self) && fmt,
             "Invalid argument.");

  axis_string_t new_str;
  axis_string_init(&new_str);

  axis_string_append_from_va_list(&new_str, fmt, ap);
  axis_string_append_formatted(&new_str, "%s", axis_string_get_raw_str(self));

  // Move the content of new_str to self.
  self->buf_size = new_str.buf_size;
  self->first_unused_idx = new_str.first_unused_idx;
  if (new_str.buf == new_str.pre_buf) {
    strcpy(self->pre_buf, new_str.pre_buf);
  } else {
    if (self->buf && self->buf != self->pre_buf) {
      axis_FREE(self->buf);
    }

    self->buf = new_str.buf;
    new_str.buf = new_str.pre_buf;
  }

  axis_string_deinit(&new_str);
}

void axis_string_prepend_formatted(axis_string_t *self, const char *fmt, ...) {
  axis_ASSERT(self && axis_string_check_integrity(self) && fmt,
             "Invalid argument.");

  va_list ap;
  va_start(ap, fmt);
  axis_string_prepend_from_va_list(self, fmt, ap);
  va_end(ap);
}

void axis_string_append_formatted(axis_string_t *self, const char *fmt, ...) {
  axis_ASSERT(self, "Invalid argument.");
  axis_ASSERT(axis_string_check_integrity(self), "Invalid argument.");
  axis_ASSERT(fmt, "Invalid argument.");

  va_list ap;
  va_start(ap, fmt);
  axis_string_append_from_va_list(self, fmt, ap);
  va_end(ap);
}

static void axis_string_reset(axis_string_t *self) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");

  if (self->buf && self->buf != self->pre_buf) {
    axis_FREE(self->buf);
    self->buf = self->pre_buf;
  }
  self->buf_size = axis_STRING_PRE_BUF_SIZE;
  self->first_unused_idx = 0;
}

void axis_string_deinit(axis_string_t *self) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");

  axis_string_reset(self);
  axis_signature_set(&self->signature, 0);
}

void axis_string_clear(axis_string_t *self) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");
  self->first_unused_idx = 0;
  self->buf[0] = 0;
}

void axis_string_reserve(axis_string_t *self, size_t extra) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");

  if (extra > SIZE_MAX - self->first_unused_idx) {
    axis_ASSERT(0, "Size overflow detected.");
    return;
  }

  size_t required_size = self->first_unused_idx + extra;
  if (required_size > (size_t)MAX_BUFFER_SIZE) {
    axis_ASSERT(0, "Buffer size exceeds the maximum limit.");
    return;
  }

  if (self->buf_size < required_size) {
    size_t new_size = self->buf_size * BUFFER_ENLARGE_RATIO;
    if (new_size < required_size) {
      new_size = required_size;
    }

    char *tmp = NULL;
    if (self->buf == self->pre_buf) {
      tmp = (char *)axis_MALLOC(new_size);
      axis_ASSERT(tmp, "Failed to allocate memory.");

      memcpy(tmp, self->buf, self->first_unused_idx);
    } else {
      tmp = (char *)axis_REALLOC(self->buf, new_size);
      axis_ASSERT(tmp, "Failed to allocate memory.");
    }

    self->buf = tmp;
    self->buf_size = new_size;
  }
}

bool axis_string_is_empty(const axis_string_t *self) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");
  return axis_c_string_is_empty(self->buf);
}

bool axis_string_starts_with(const axis_string_t *self, const char *start) {
  axis_ASSERT(self && axis_string_check_integrity(self) && start,
             "Invalid argument.");

  int index = axis_c_string_index_of(axis_string_get_raw_str(self), start);
  return index == 0;
}

void axis_string_erase_back(axis_string_t *self, size_t count) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");

  size_t len = strlen(self->buf);
  if (count > len) {
    count = len;
  }
  self->buf[len - count] = '\0';
}

// Compare 'a' & 'b' up to 'num' characters.
static int strcmp_case_insensitive(const char *a, const char *b, size_t num) {
  for (size_t i = 0; i < num; a++, b++, i++) {
    int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
    if (d != 0 || !*a) {
      return d;
    }
  }
  return 0;
}

bool axis_c_string_is_equal(const char *a, const char *b) {
  axis_ASSERT(a && b, "Invalid argument.");
  return (0 == strcmp(a, b)) ? true : false;
}

bool axis_c_string_is_equal_with_size(const char *a, const char *b, size_t num) {
  axis_ASSERT(a && b, "Invalid argument.");
  return (0 == strncmp(a, b, num)) ? true : false;
}

bool axis_c_string_is_equal_case_insensitive(const char *a, const char *b) {
  axis_ASSERT(a && b, "Invalid argument.");
  return (0 == strcmp_case_insensitive(a, b, strlen(a))) ? true : false;
}

bool axis_c_string_is_equal_with_size_case_insensitive(const char *a,
                                                      const char *b,
                                                      size_t num) {
  axis_ASSERT(a && b, "Invalid argument.");
  return (0 == strcmp_case_insensitive(a, b, num)) ? true : false;
}

bool axis_c_string_contains(const char *a, const char *b) {
  axis_ASSERT(a && b, "Invalid argument.");
  return strstr(a, b) != NULL;
}

bool axis_string_is_equal(const axis_string_t *a, const axis_string_t *b) {
  axis_ASSERT(
      a && axis_string_check_integrity(a) && b && axis_string_check_integrity(b),
      "Invalid argument.");
  return axis_c_string_is_equal(axis_string_get_raw_str(a),
                               axis_string_get_raw_str(b));
}

bool axis_string_is_equal_c_str(axis_string_t *a, const char *b) {
  axis_ASSERT(a && axis_string_check_integrity(a) && b, "Invalid argument.");
  return axis_c_string_is_equal(axis_string_get_raw_str(a), b);
}

bool axis_string_is_equal_c_str_case_insensitive(axis_string_t *a,
                                                const char *b) {
  axis_ASSERT(a && axis_string_check_integrity(a) && b, "Invalid argument.");
  return axis_c_string_is_equal_case_insensitive(axis_string_get_raw_str(a), b);
}

bool axis_string_contains(axis_string_t *self, const char *b) {
  axis_ASSERT(self && axis_string_check_integrity(self) && b,
             "Invalid argument.");
  return axis_c_string_contains(axis_string_get_raw_str(self), b);
}

void axis_string_to_lower(axis_string_t *self) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");
  for (size_t i = 0; i < axis_string_len(self); ++i) {
    self->buf[i] = tolower(self->buf[i]);
  }
}

void axis_string_to_upper(axis_string_t *self) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");
  for (size_t i = 0; i < axis_string_len(self); ++i) {
    self->buf[i] = toupper(self->buf[i]);
  }
}

bool axis_c_string_is_empty(const char *str) {
  axis_ASSERT(str, "Invalid argument.");
  return str[0] == 0;
}

bool axis_c_string_starts_with(const char *str, const char *prefix) {
  axis_ASSERT(str && prefix, "Invalid argument.");
  return strncmp(prefix, str, strlen(prefix)) == 0;
}

bool axis_c_string_ends_with(const char *str, const char *postfix) {
  axis_ASSERT(str && postfix, "Invalid argument.");
  return strncmp(postfix, str + strlen(str) - strlen(postfix),
                 strlen(postfix)) == 0;
}

bool axis_c_string_is_equal_or_smaller(const char *a, const char *b) {
  axis_ASSERT(a && b, "Invalid argument.");

  if (strlen(a) < strlen(b)) {
    return true;
  } else if (strlen(a) > strlen(b)) {
    return false;
  } else {
    for (size_t i = 0; i < strlen(a); ++i) {
      if (a[i] < b[i]) {
        return true;
      } else if (a[i] > b[i]) {
        return false;
      }
    }
    return true;
  }
}

int axis_c_string_index_of(const char *src, const char *search) {
  axis_ASSERT(src && search, "Invalid argument.");

  const char *match = strstr(src, search);
  if (match) {
    return match - src;
  } else {
    return -1;
  }
}

void axis_c_string_split(const char *src, const char *delimiter,
                        axis_list_t *result) {
  axis_ASSERT(src && delimiter && result, "Invalid argument.");

  // Because strtok would modify the string content, we need to clone the
  // string first.
  char *tmp = (char *)axis_MALLOC(strlen(src) + 1);
  axis_ASSERT(tmp, "Failed to allocate memory.");

  memcpy(tmp, src, strlen(src) + 1);

  char *save = NULL;
  char *token = strtok_r(tmp, delimiter, &save);
  while (token) {
    axis_list_push_str_back(result, token);
    token = strtok_r(NULL, delimiter, &save);
  }

  axis_FREE(tmp);
}

void axis_string_split(axis_string_t *self, const char *delimiter,
                      axis_list_t *result) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");
  axis_ASSERT(delimiter, "Invalid argument.");
  axis_ASSERT(result && axis_list_check_integrity(result), "Invalid argument.");

  return axis_c_string_split(axis_string_get_raw_str(self), delimiter, result);
}

static const char *uri_encode_tbl[0x100] = {
    "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07", "%08", "%09", "%0A",
    "%0B", "%0C", "%0D", "%0E", "%0F", "%10", "%11", "%12", "%13", "%14", "%15",
    "%16", "%17", "%18", "%19", "%1A", "%1B", "%1C", "%1D", "%1E", "%1F", "%20",
    "%21", "%22", "%23", "%24", "%25", "%26", "%27", "%28", "%29", "%2A", "%2B",
    "%2C", NULL,  NULL,  "%2F", NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,
    NULL,  NULL,  NULL,  "%3A", "%3B", "%3C", "%3D", "%3E", "%3F", "%40", NULL,
    NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,
    NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,
    NULL,  NULL,  NULL,  "%5B", "%5C", "%5D", "%5E", NULL,  "%60", NULL,  NULL,
    NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,
    NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,  NULL,
    NULL,  NULL,  "%7B", "%7C", "%7D", NULL,  "%7F", "%80", "%81", "%82", "%83",
    "%84", "%85", "%86", "%87", "%88", "%89", "%8A", "%8B", "%8C", "%8D", "%8E",
    "%8F", "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97", "%98", "%99",
    "%9A", "%9B", "%9C", "%9D", "%9E", "%9F", "%A0", "%A1", "%A2", "%A3", "%A4",
    "%A5", "%A6", "%A7", "%A8", "%A9", "%AA", "%AB", "%AC", "%AD", "%AE", "%AF",
    "%B0", "%B1", "%B2", "%B3", "%B4", "%B5", "%B6", "%B7", "%B8", "%B9", "%BA",
    "%BB", "%BC", "%BD", "%BE", "%BF", "%C0", "%C1", "%C2", "%C3", "%C4", "%C5",
    "%C6", "%C7", "%C8", "%C9", "%CA", "%CB", "%CC", "%CD", "%CE", "%CF", "%D0",
    "%D1", "%D2", "%D3", "%D4", "%D5", "%D6", "%D7", "%D8", "%D9", "%DA", "%DB",
    "%DC", "%DD", "%DE", "%DF", "%E0", "%E1", "%E2", "%E3", "%E4", "%E5", "%E6",
    "%E7", "%E8", "%E9", "%EA", "%EB", "%EC", "%ED", "%EE", "%EF", "%F0", "%F1",
    "%F2", "%F3", "%F4", "%F5", "%F6", "%F7", "%F8", "%F9", "%FA", "%FB", "%FC",
    "%FD", "%FE", "%FF",
};

#define __ 0xFF
static const unsigned char hex_val_table[0x100] = {
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 00-0F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 10-1F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 20-2F */
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  __, __, __, __, __, __, /* 30-3F */
    __, 10, 11, 12, 13, 14, 15, __, __, __, __, __, __, __, __, __, /* 40-4F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 50-5F */
    __, 10, 11, 12, 13, 14, 15, __, __, __, __, __, __, __, __, __, /* 60-6F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 70-7F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 80-8F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* 90-9F */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* A0-AF */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* B0-BF */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* C0-CF */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* D0-DF */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* E0-EF */
    __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, /* F0-FF */
};
#undef __

void axis_c_string_uri_encode(const char *src, const size_t len,
                             axis_string_t *result) {
  axis_ASSERT(src && result && axis_string_check_integrity(result),
             "Invalid argument.");

  size_t i = 0;
  while (i < len) {
    const char octet = src[i++];
    const char *code = uri_encode_tbl[(int8_t)octet];
    if (code) {
      axis_string_append_formatted(result, "%s", code);
    } else {
      axis_string_append_formatted(result, "%c", octet);
    }
  }
}

void axis_c_string_uri_decode(const char *src, const size_t len,
                             axis_string_t *result) {
  axis_ASSERT(result && axis_string_check_integrity(result), "Invalid argument.");

  size_t i = 0;
  while (i < len) {
    bool copy_char = true;
    if (src[i] == '%' && i + 2 < len) {
      const unsigned char v1 = hex_val_table[(unsigned char)src[i + 1]];
      const unsigned char v2 = hex_val_table[(unsigned char)src[i + 2]];

      // skip invalid hex sequences
      if ((v1 | v2) != 0xFF) {
        axis_string_append_formatted(result, "%c", (v1 << 4) | v2);
        i += 3;
        copy_char = false;
      }
    }
    if (copy_char) {
      axis_string_append_formatted(result, "%c", src[i]);
      i++;
    }
  }
}

void axis_string_hex_from_buf(axis_string_t *self, axis_buf_t buf) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");

  for (size_t i = 0; i < buf.content_size; i++) {
    axis_string_append_formatted(self, "%02x", ((uint8_t *)(buf.data))[i]);
  }
}

void axis_string_trim_trailing_slash(axis_string_t *self) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");

  for (size_t i = axis_string_len(self) - 1;
       i >= 0 && ((self->buf[i] == '/') || (self->buf[i] == '\\')); i--) {
    self->buf[i] = '\0';
    self->first_unused_idx--;
  }
}

void axis_string_trim_trailing_whitespace(axis_string_t *self) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");

  for (size_t i = axis_string_len(self) - 1; i >= 0 && isspace(self->buf[i]);
       i--) {
    self->buf[i] = '\0';
    self->first_unused_idx--;
  }
}

void axis_string_trim_leading_whitespace(axis_string_t *self) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");

  size_t whitespace_cnt = 0;
  for (size_t i = 0; i < axis_string_len(self); i++) {
    if (isspace(self->buf[i])) {
      whitespace_cnt++;
    } else {
      break;
    }
  }

  if (whitespace_cnt) {
    memmove(self->buf, self->buf + whitespace_cnt,
            self->first_unused_idx - whitespace_cnt);
    self->first_unused_idx -= whitespace_cnt;
    self->buf[self->first_unused_idx] = '\0';
  }
}

char *axis_c_string_trim_trailing_whitespace(char *self) {
  axis_ASSERT(self, "Invalid argument.");

  for (size_t i = strlen(self) - 1; i >= 0 && isspace(self[i]); i--) {
    self[i] = '\0';
  }
  return self;
}

void axis_c_string_escaped(const char *src, axis_string_t *result) {
  axis_ASSERT(src && result && axis_string_check_integrity(result),
             "Invalid argument.");

  for (size_t i = 0; i < strlen(src); i++) {
    char byte = src[i];
    switch (byte) {
      case 0x08: {  // backspace
        axis_string_append_formatted(result, "%c", '\\');
        axis_string_append_formatted(result, "%c", 'b');
        break;
      }
      case 0x09: {  // horizontal tab
        axis_string_append_formatted(result, "%c", '\\');
        axis_string_append_formatted(result, "%c", 't');
        break;
      }
      case 0x0A: {  // newline
        axis_string_append_formatted(result, "%c", '\\');
        axis_string_append_formatted(result, "%c", 'n');
        break;
      }
      case 0x0C: {  // formfeed
        axis_string_append_formatted(result, "%c", '\\');
        axis_string_append_formatted(result, "%c", 'f');
        break;
      }
      case 0x0D: {  // carriage return
        axis_string_append_formatted(result, "%c", '\\');
        axis_string_append_formatted(result, "%c", 'r');
        break;
      }
      case 0x22: {  // quotation mark
        axis_string_append_formatted(result, "%c", '\\');
        axis_string_append_formatted(result, "%c", '\"');
        break;
      }
      case 0x5C: {  // reverse solidus
        axis_string_append_formatted(result, "%c", '\\');
        axis_string_append_formatted(result, "%c", '\\');
        break;
      }
      default: {
        // escape control characters (0x00..0x1F) or, if
        // ensure_ascii parameter is used, non-ASCII characters
        if (byte <= 0x1F) {
          axis_string_append_formatted(result, "\\u%04x", byte);
        } else {
          axis_string_append_formatted(result, "%c", byte);
        }
        break;
      }
    }
  }
}

bool axis_string_is_uuid4(axis_string_t *self) {
  axis_ASSERT(self && axis_string_check_integrity(self), "Invalid argument.");

  if (axis_string_len(self) != 36) {
    return false;
  }

  for (size_t i = 0; i < axis_string_len(self); ++i) {
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (axis_string_get_raw_str(self)[i] != '-') {
        return false;
      }
    } else {
      if (axis_string_get_raw_str(self)[i] < '0' ||
          axis_string_get_raw_str(self)[i] > 'f') {
        return false;
      }
    }
  }

  return true;
}

void axis_string_slice(axis_string_t *self, axis_string_t *other, char sep) {
  axis_ASSERT(self && axis_string_check_integrity(self) && other &&
                 axis_string_check_integrity(other),
             "Invalid argument.");

  // Find separator in buf.
  char *pr = strchr(axis_string_get_raw_str(self), sep);
  if (pr == NULL) {
    return;
  }

  // Length from separator to end of buf.
  size_t plen = strlen(pr);

  // Copy from self into 'other' string.
  axis_string_init_formatted(other, "%.*s", plen, pr + 1);

  *pr = '\0';
  self->first_unused_idx = (pr - self->buf);
}

extern inline const char *axis_string_get_raw_str(const axis_string_t *self);

extern inline size_t axis_string_len(const axis_string_t *self);

extern inline bool axis_string_check_integrity(const axis_string_t *self);
