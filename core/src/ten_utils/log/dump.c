//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/ten_config.h"

#include <ctype.h>
#include <string.h>
#include <time.h>

#include "include_internal/ten_utils/log/buffer.h"
#include "include_internal/ten_utils/log/format.h"
#include "include_internal/ten_utils/log/internal.h"
#include "include_internal/ten_utils/log/level.h"
#include "include_internal/ten_utils/log/pid.h"
#include "include_internal/ten_utils/log/time.h"
#include "ten_utils/lib/buf.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"

#ifndef _countof
  #define _countof(xs) (sizeof(xs) / sizeof((xs)[0]))
#endif

static const char c_hex[] = "0123456789abcdef";

/**
 * @brief Put @a padding_char reversely into the buffer specified by @a end
 * which points to the end of the buffer.
 *
 *         |<--   width   -->|
 *         |-----------------|
 *          cccccc^         end
 *               ptr
 */
static char *put_padding_r(const size_t width, const char padding_char,
                           char *ptr, char *end) {
  for (char *begin = end - width; begin < ptr; *--ptr = padding_char) {
  }
  return ptr;
}

/**
 * @brief Put @a value reversely into a buffer specified by @a end points to the
 * end of the buffer.
 *
 *         |<--   width   -->|
 *         |-----------------|
 *                          end
 *                   value
 *          padding
 */
static char *put_integer_r(uint64_t value, const TEN_SIGN sign,
                           const size_t width, const char padding_char,
                           char *end) {
  static const char _signs[] = {'-', '0', '+'};
  static const char *signs = _signs + 1;

  char *ptr = end;

  // Dump 'value'.
  do {
    *--ptr = (char)('0' + (value % 10));
  } while (0 != (value /= 10));

  if (sign == TEN_SIGN_ZERO) {
    return put_padding_r(width, padding_char, ptr, end);
  }

  if ('0' != padding_char) {
    *--ptr = signs[sign];
    return put_padding_r(width, padding_char, ptr, end);
  }

  ptr = put_padding_r(width, padding_char, ptr, end + 1);
  *--ptr = signs[sign];

  return ptr;
}

/**
 * @brief Put @a value reversely into a buffer specified by @a end points to the
 * end of the buffer.
 *
 *         |<--   width   -->|
 *         |-----------------|
 *                          end
 *                   value
 *          padding
 */
static char *put_int_r(const int64_t value, const size_t width,
                       const char padding_char, char *end) {
  return 0 <= value
             ? put_integer_r((uint64_t)value, 0, width, padding_char, end)
             : put_integer_r((uint64_t)-value, -1, width, padding_char, end);
}

static char *put_uint_r(const uint64_t value, const size_t width,
                        const char padding_char, char *end) {
  return put_integer_r(value, TEN_SIGN_ZERO, width, padding_char, end);
}

/**
 * @brief Put a string into a buffer specified by @a end points to the end of
 * the buffer.
 *
 *         |-----------------|
 *        ptr               end
 *         ^           ^
 *     str_begin    str_end
 */
static char *put_stringn(const char *str_begin, const char *str_end, char *ptr,
                         const char *end) {
  const ptrdiff_t m = end - ptr;
  ptrdiff_t n = str_end - str_begin;

  // Check if the string is larger than the buffer.
  if (n > m) {
    n = m;
  }

  memcpy(ptr, str_begin, n);

  return ptr + n;
}

/**
 * @brief Put @a value into a buffer specified by @a end points to the end of
 * the buffer.
 *
 *         |<-- width -->|
 *          padding
 *                  value
 *         |------------------|
 *        ptr                end
 */
static char *put_uint(uint64_t value, const size_t width,
                      const char padding_char, char *ptr, char *end) {
  char buf[16];
  char *const buf_end = buf + _countof(buf);
  char *str_begin = put_uint_r(value, width, padding_char, buf_end);
  return put_stringn(str_begin, buf_end, ptr, end);
}

/**
 * @brief Put @a str into a buffer specified by @a ptr to @a end.
 *
 *         |------------------|
 *        ptr                end
 *          str->
 */
static char *put_string(const char *str, char *ptr, char *end) {
  const ptrdiff_t n = end - ptr;
  char *const c = (char *)memccpy(ptr, str, '\0', n);
  return 0 != c ? c - 1 : end;
}

/**
 * @brief Move the end of the log message content to @a n characters forward,
 * and do not exceed the end of the log buffer.
 */
static void put_nprintf(ten_log_message_t *log_msg, const int n) {
  TEN_ASSERT(log_msg, "Invalid argument.");

  if (n > 0) {
    log_msg->buf_content_end = n < log_msg->buf_end - log_msg->buf_content_end
                                   ? log_msg->buf_content_end + n
                                   : log_msg->buf_end;
  }
}

/**
 * @brief *nprintf() (ex: vsnprintf) always puts 0 in the end when input buffer
 * is not empty. This 0 is not desired because its presence sets (ctx->p) to
 * (ctx->e - 1) which leaves space for one more character. Some put_xxx()
 * functions don't use *nprintf() and could use that last character. In that
 * case log line will have multiple (two) half-written parts which is confusing.
 * To workaround that we allow *nprintf() to write its 0 in the eol area (which
 * is always not empty).
 */
static size_t nprintf_size(ten_log_message_t *log_msg) {
  TEN_ASSERT(log_msg, "Invalid argument.");

  return (size_t)(log_msg->buf_end - log_msg->buf_content_end + 1);
}

const char *funcname(const char *func) { return func ? func : ""; }

const char *filename(const char *file) {
  const char *f = file;
  for (const char *p = file; 0 != *p; ++p) {
    if ('/' == *p || '\\' == *p) {
      f = p + 1;
    }
  }
  return f;
}

/**
 * @brief Put log context to @a log_msg.
 */
static void put_ctx(ten_log_message_t *log_msg) {
  TEN_ASSERT(log_msg, "Invalid argument.");

  _PP_MAP(_TEN_LOG_MESSAGE_FORMAT_INIT, TEN_LOG_MESSAGE_CTX_FORMAT)

#if !_TEN_LOG_MESSAGE_FORMAT_FIELDS(TEN_LOG_MESSAGE_CTX_FORMAT)
  VAR_UNUSED(msg);
#else

  #if _TEN_LOG_MESSAGE_FORMAT_DATETIME_USED
  struct tm tm;
  size_t msec = 0;
  g_ten_log_get_time(&tm, &msec);
  #endif

  #if _TEN_LOG_MESSAGE_FORMAT_CONTAINS(PID, TEN_LOG_MESSAGE_CTX_FORMAT) || \
      _TEN_LOG_MESSAGE_FORMAT_CONTAINS(TID, TEN_LOG_MESSAGE_CTX_FORMAT)
  int64_t pid = 0;
  int64_t tid = 0;
  ten_log_get_pid_tid(&pid, &tid);
  #endif

  char ctx_buf[64];
  char *end = ctx_buf + sizeof(ctx_buf);
  char *ptr = end;

  _PP_RMAP(_TEN_LOG_MESSAGE_FORMAT_PUT_R, TEN_LOG_MESSAGE_CTX_FORMAT)

  log_msg->buf_content_end =
      put_stringn(ptr, end, log_msg->buf_content_end, log_msg->buf_end);

#endif  // !_TEN_LOG_MESSAGE_FORMAT_FIELDS(TEN_LOG_MESSAGE_CTX_FORMAT)
}

/**
 * @brief Put log @a tag to @a log_msg.
 */
static void put_tag(ten_log_message_t *log_msg, const char *tag) {
  TEN_ASSERT(log_msg, "Invalid argument.");

  _PP_MAP(_TEN_LOG_MESSAGE_FORMAT_INIT, TEN_LOG_MESSAGE_TAG_FORMAT)

#if !_TEN_LOG_MESSAGE_FORMAT_CONTAINS(TAG, TEN_LOG_MESSAGE_TAG_FORMAT)
  VAR_UNUSED(tag);
#endif

#if !_TEN_LOG_MESSAGE_FORMAT_FIELDS(TEN_LOG_MESSAGE_TAG_FORMAT)
  VAR_UNUSED(log_msg);
#else

  _PP_MAP(_TEN_LOG_MESSAGE_FORMAT_PUT, TEN_LOG_MESSAGE_TAG_FORMAT)

#endif
}

static void put_src(ten_log_message_t *const log_msg,
                    const ten_log_src_location_t *src_loc) {
  TEN_ASSERT(log_msg, "Invalid argument.");

  _PP_MAP(_TEN_LOG_MESSAGE_FORMAT_INIT, TEN_LOG_MESSAGE_SRC_FORMAT)

#if !_TEN_LOG_MESSAGE_FORMAT_CONTAINS(FUNCTION, TEN_LOG_MESSAGE_SRC_FORMAT) && \
    !_TEN_LOG_MESSAGE_FORMAT_CONTAINS(FILENAME, TEN_LOG_MESSAGE_SRC_FORMAT) && \
    !_TEN_LOG_MESSAGE_FORMAT_CONTAINS(FILELINE, TEN_LOG_MESSAGE_SRC_FORMAT)
  VAR_UNUSED(src);
#endif

#if !_TEN_LOG_MESSAGE_FORMAT_FIELDS(TEN_LOG_MESSAGE_SRC_FORMAT)
  VAR_UNUSED(msg);
#else

  _PP_MAP(_TEN_LOG_MESSAGE_FORMAT_PUT, TEN_LOG_MESSAGE_SRC_FORMAT)

#endif
}

static void put_msg(ten_log_message_t *log_msg, const char *fmt, va_list va) {
  TEN_ASSERT(log_msg, "Invalid argument.");

  int n = 0;
  log_msg->msg_start = log_msg->buf_content_end;

  // 'n' is the number of characters that would have been written if n had been
  // sufficiently large, not counting the terminating null character.
  n = vsnprintf(log_msg->buf_content_end, nprintf_size(log_msg), fmt, va);

  put_nprintf(log_msg, n);
}

static void output_mem(const ten_log_t *log, ten_log_message_t *log_msg,
                       const ten_buf_t *mem) {
  TEN_ASSERT(mem, "Invalid argument.");

  if (mem->data == NULL || mem->content_size == 0) {
    return;
  }

  const char *mem_p = (const char *)mem->data;
  const char *mem_e = mem_p + mem->content_size;
  const char *mem_cut = NULL;
  const ptrdiff_t mem_width = (ptrdiff_t)log->format->mem_width;
  char *const hex_b = log_msg->msg_start;
  char *const ascii_b = hex_b + 2 * mem_width + 2;
  char *const ascii_e = ascii_b + mem_width;

  if (log_msg->buf_end < ascii_e) {
    return;
  }

  while (mem_p != mem_e) {
    char *hex = hex_b;
    char *ascii = ascii_b;
    for (mem_cut = mem_width < mem_e - mem_p ? mem_p + mem_width : mem_e;
         mem_cut != mem_p; ++mem_p) {
      const unsigned char ch = *mem_p;
      *hex++ = c_hex[(0xf0 & ch) >> 4];
      *hex++ = c_hex[(0x0f & ch)];
      *ascii++ = isprint(ch) ? (char)ch : '?';
    }

    while (hex != ascii_b) {
      *hex++ = ' ';
    }

    log_msg->buf_content_end = ascii;
    log->output->output_cb(log_msg, log->output->arg);
  }
}

void ten_log_write_imp(const ten_log_t *log,
                       const ten_log_src_location_t *src_loc,
                       const ten_buf_t *mem, const TEN_LOG_LEVEL level,
                       const char *tag, const char *fmt, va_list va) {
  TEN_ASSERT(log && fmt, "Invalid argument.");

  ten_log_message_t log_msg;
  char buf[TEN_LOG_BUF_SZ];
  const uint64_t mask = log->output->mask;
  log_msg.level = level;
  log_msg.tag = tag;

  g_buffer_cb(&log_msg, buf);

  if (TEN_LOG_PUT_CTX & mask) {
    put_ctx(&log_msg);
  }
  if (TEN_LOG_PUT_TAG & mask) {
    put_tag(&log_msg, tag);
  }
  if (0 != src_loc && TEN_LOG_PUT_SRC & mask) {
    put_src(&log_msg, src_loc);
  }
  if (TEN_LOG_PUT_MSG & mask) {
    put_msg(&log_msg, fmt, va);
  }

  log->output->output_cb(&log_msg, log->output->arg);

  if (0 != mem && TEN_LOG_PUT_MSG & mask) {
    output_mem(log, &log_msg, mem);
  }
}
