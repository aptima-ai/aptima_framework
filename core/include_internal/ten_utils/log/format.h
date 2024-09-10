//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

/**
 * @brief Fields that can be used in log message format specifications.
 * Mentioning them here explicitly, so we know that nobody else defined them
 * before us. See TEN_LOG_MESSAGE_CTX_FORMAT for details.
 */
#define YEAR YEAR
#define MONTH MONTH
#define DAY DAY
#define MINUTE MINUTE
#define SECOND SECOND
#define MILLISECOND MILLISECOND
#define PID PID
#define TID TID
#define LEVEL LEVEL
#define TAG(prefix_delim, tag_delim) TAG(prefix_delim, tag_delim)
#define FUNCTION FUNCTION
#define FILENAME FILENAME
#define FILELINE FILELINE
#define S(str) S(str)
#define F_INIT(statements) F_INIT(statements)
#define F_UINT(width, value) F_UINT(width, value)

#define _PP_PASTE_2(a, b) a##b
#define _PP_CONCAT_2(a, b) _PP_PASTE_2(a, b)

#define _PP_PASTE_3(a, b, c) a##b##c
#define _PP_CONCAT_3(a, b, c) _PP_PASTE_3(a, b, c)

#define _PP_ID(x) x

/**
 * @brief Get the number of the arguments.
 *
 * @note Microsoft C preprocessor is a piece of shit. This moron treats
 * __VA_ARGS__ as a single token and requires additional expansion to realize
 * that it's actually a list. If not for it, there would be no need in this
 * extra expansion.
 */
#define _PP_NARGS_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, \
                    _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, \
                    _24, ...)                                              \
  _24
#define _PP_NARGS(...)                                                        \
  _PP_ID(_PP_NARGS_N(__VA_ARGS__, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, \
                     13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

/**
 * @brief Get the 1st argument.
 *
 * @a xs must be enclosed into additional "()".
 *
 * @note There is a more efficient way to implement this, but it requires
 * working C preprocessor. Unfortunately, Microsoft Visual Studio doesn't have
 * one.
 */
#define _PP_HEAD__(x, ...) x
#define _PP_HEAD_(...) _PP_ID(_PP_HEAD__(__VA_ARGS__, ~))
#define _PP_HEAD(xs) _PP_HEAD_ xs

/**
 * @brief Remove the 1st argument.
 *
 * @a xs must be enclosed into additional "()".
 */
#define _PP_TAIL_(x, ...) (__VA_ARGS__)
#define _PP_TAIL(xs) _PP_TAIL_ xs

/**
 * @brief Remove the enclosing "()".
 *
 * @a xs must be enclosed into additional "()".
 */
#define _PP_UNTUPLE_(...) __VA_ARGS__
#define _PP_UNTUPLE(xs) _PP_UNTUPLE_ xs

/**
 * @brief F_INIT field support. All possible fields must be mentioned here.
 *
 * Ex:
 * - '#define _TEN_LOG_MESSAGE_FORMAT_INIT__YEAR' means no special handling is
 *   needed for 'YEAR'
 *
 * - '#define _TEN_LOG_MESSAGE_FORMAT_INIT__F_INIT(expr) _PP_UNTUPLE(expr);'
 *   means if there is any 'F_init(expr)' in the format specifications, then the
 *   'expr' will be flattened.
 */
#define _TEN_LOG_MESSAGE_FORMAT_INIT__
#define _TEN_LOG_MESSAGE_FORMAT_INIT__YEAR
#define _TEN_LOG_MESSAGE_FORMAT_INIT__MONTH
#define _TEN_LOG_MESSAGE_FORMAT_INIT__DAY
#define _TEN_LOG_MESSAGE_FORMAT_INIT__HOUR
#define _TEN_LOG_MESSAGE_FORMAT_INIT__MINUTE
#define _TEN_LOG_MESSAGE_FORMAT_INIT__SECOND
#define _TEN_LOG_MESSAGE_FORMAT_INIT__MILLISECOND
#define _TEN_LOG_MESSAGE_FORMAT_INIT__PID
#define _TEN_LOG_MESSAGE_FORMAT_INIT__TID
#define _TEN_LOG_MESSAGE_FORMAT_INIT__LEVEL
#define _TEN_LOG_MESSAGE_FORMAT_INIT__TAG(ps, ts)
#define _TEN_LOG_MESSAGE_FORMAT_INIT__FUNCTION
#define _TEN_LOG_MESSAGE_FORMAT_INIT__FILENAME
#define _TEN_LOG_MESSAGE_FORMAT_INIT__FILELINE
#define _TEN_LOG_MESSAGE_FORMAT_INIT__S(s)
#define _TEN_LOG_MESSAGE_FORMAT_INIT__F_INIT(expr) _PP_UNTUPLE(expr);
#define _TEN_LOG_MESSAGE_FORMAT_INIT__F_UINT(w, v)
#define _TEN_LOG_MESSAGE_FORMAT_INIT(field) \
  _PP_CONCAT_3(_TEN_LOG_MESSAGE_FORMAT_INIT_, _, field)

/**
 * @brief Used to implement _TEN_LOG_MESSAGE_FORMAT_CONTAINS() macro. All
 * possible fields must be mentioned here. Not counting F_INIT() here because
 * it's somewhat special and is handled separately (at least for now).
 */
#define _TEN_LOG_MESSAGE_FORMAT_MASK__ (0 << 0)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__YEAR (1 << 1)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__MONTH (1 << 2)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__DAY (1 << 3)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__HOUR (1 << 4)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__MINUTE (1 << 5)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__SECOND (1 << 6)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__MILLISECOND (1 << 7)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__PID (1 << 8)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__TID (1 << 9)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__LEVEL (1 << 10)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__TAG(ps, ts) (1 << 11)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__FUNCTION (1 << 12)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__FILENAME (1 << 13)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__FILELINE (1 << 14)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__S(s) (1 << 15)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__F_INIT(expr) (0 << 16)
#define _TEN_LOG_MESSAGE_FORMAT_MASK__F_UINT(w, v) (1 << 17)
#define _TEN_LOG_MESSAGE_FORMAT_MASK(field) \
  _PP_CONCAT_3(_TEN_LOG_MESSAGE_FORMAT_MASK_, _, field)

/**
 * @brief Logical "or" of masks of fields used in specified format
 * specification.
 */
#define _TEN_LOG_MESSAGE_FORMAT_FIELDS(format) \
  (0 _PP_MAP(| _TEN_LOG_MESSAGE_FORMAT_MASK, format))

/**
 * @brief Apply function macro to each element in tuple. Output is not
 * enforced to be a tuple.
 *
 * @a f function macro
 * @a xs must be enclosed into additional "()".
 *
 * @note the X of _PP_MAP_X means the amount of the arguments.
 */
#define _PP_MAP_1(f, xs) f(_PP_HEAD(xs))
#define _PP_MAP_2(f, xs) f(_PP_HEAD(xs)) _PP_MAP_1(f, _PP_TAIL(xs))
#define _PP_MAP_3(f, xs) f(_PP_HEAD(xs)) _PP_MAP_2(f, _PP_TAIL(xs))
#define _PP_MAP_4(f, xs) f(_PP_HEAD(xs)) _PP_MAP_3(f, _PP_TAIL(xs))
#define _PP_MAP_5(f, xs) f(_PP_HEAD(xs)) _PP_MAP_4(f, _PP_TAIL(xs))
#define _PP_MAP_6(f, xs) f(_PP_HEAD(xs)) _PP_MAP_5(f, _PP_TAIL(xs))
#define _PP_MAP_7(f, xs) f(_PP_HEAD(xs)) _PP_MAP_6(f, _PP_TAIL(xs))
#define _PP_MAP_8(f, xs) f(_PP_HEAD(xs)) _PP_MAP_7(f, _PP_TAIL(xs))
#define _PP_MAP_9(f, xs) f(_PP_HEAD(xs)) _PP_MAP_8(f, _PP_TAIL(xs))
#define _PP_MAP_10(f, xs) f(_PP_HEAD(xs)) _PP_MAP_9(f, _PP_TAIL(xs))
#define _PP_MAP_11(f, xs) f(_PP_HEAD(xs)) _PP_MAP_10(f, _PP_TAIL(xs))
#define _PP_MAP_12(f, xs) f(_PP_HEAD(xs)) _PP_MAP_11(f, _PP_TAIL(xs))
#define _PP_MAP_13(f, xs) f(_PP_HEAD(xs)) _PP_MAP_12(f, _PP_TAIL(xs))
#define _PP_MAP_14(f, xs) f(_PP_HEAD(xs)) _PP_MAP_13(f, _PP_TAIL(xs))
#define _PP_MAP_15(f, xs) f(_PP_HEAD(xs)) _PP_MAP_14(f, _PP_TAIL(xs))
#define _PP_MAP_16(f, xs) f(_PP_HEAD(xs)) _PP_MAP_15(f, _PP_TAIL(xs))
#define _PP_MAP_17(f, xs) f(_PP_HEAD(xs)) _PP_MAP_16(f, _PP_TAIL(xs))
#define _PP_MAP_18(f, xs) f(_PP_HEAD(xs)) _PP_MAP_17(f, _PP_TAIL(xs))
#define _PP_MAP_19(f, xs) f(_PP_HEAD(xs)) _PP_MAP_18(f, _PP_TAIL(xs))
#define _PP_MAP_20(f, xs) f(_PP_HEAD(xs)) _PP_MAP_19(f, _PP_TAIL(xs))
#define _PP_MAP_21(f, xs) f(_PP_HEAD(xs)) _PP_MAP_20(f, _PP_TAIL(xs))
#define _PP_MAP_22(f, xs) f(_PP_HEAD(xs)) _PP_MAP_21(f, _PP_TAIL(xs))
#define _PP_MAP_23(f, xs) f(_PP_HEAD(xs)) _PP_MAP_22(f, _PP_TAIL(xs))
#define _PP_MAP_24(f, xs) f(_PP_HEAD(xs)) _PP_MAP_23(f, _PP_TAIL(xs))
#define _PP_MAP(f, xs) _PP_CONCAT_2(_PP_MAP_, _PP_NARGS xs)(f, xs)

/**
 * @brief Apply function macro to each element in tuple in reverse order.
 * Output is not enforced to be a tuple.
 *
 * @a f function macro
 * @a xs must be enclosed into additional "()".
 *
 * @note the X of _PP_RMAP_X means the amount of the arguments.
 */
#define _PP_RMAP_1(f, xs) f(_PP_HEAD(xs))
#define _PP_RMAP_2(f, xs) _PP_RMAP_1(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_3(f, xs) _PP_RMAP_2(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_4(f, xs) _PP_RMAP_3(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_5(f, xs) _PP_RMAP_4(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_6(f, xs) _PP_RMAP_5(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_7(f, xs) _PP_RMAP_6(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_8(f, xs) _PP_RMAP_7(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_9(f, xs) _PP_RMAP_8(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_10(f, xs) _PP_RMAP_9(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_11(f, xs) _PP_RMAP_10(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_12(f, xs) _PP_RMAP_11(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_13(f, xs) _PP_RMAP_12(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_14(f, xs) _PP_RMAP_13(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_15(f, xs) _PP_RMAP_14(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_16(f, xs) _PP_RMAP_15(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_17(f, xs) _PP_RMAP_16(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_18(f, xs) _PP_RMAP_17(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_19(f, xs) _PP_RMAP_18(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_20(f, xs) _PP_RMAP_19(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_21(f, xs) _PP_RMAP_20(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_22(f, xs) _PP_RMAP_21(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_23(f, xs) _PP_RMAP_22(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP_24(f, xs) _PP_RMAP_23(f, _PP_TAIL(xs)) f(_PP_HEAD(xs))
#define _PP_RMAP(f, xs) _PP_CONCAT_2(_PP_RMAP_, _PP_NARGS xs)(f, xs)

#define PUT_CSTR_R(ptr, STR)                     \
  do {                                           \
    for (size_t i = sizeof(STR) - 1; 0 < i--;) { \
      *--(ptr) = (STR)[i];                       \
    }                                            \
  } while (0)

/**
 * @brief Implements generation of put_xxx_r statements for log message
 * specification.
 */
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__YEAR \
  ptr = put_uint_r(tm.tm_year + 1900, 4, '0', ptr);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__MONTH \
  ptr = put_uint_r((uint64_t)tm.tm_mon + 1, 2, '0', ptr);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__DAY \
  ptr = put_uint_r((uint64_t)tm.tm_mday, 2, '0', ptr);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__HOUR \
  ptr = put_uint_r((uint64_t)tm.tm_hour, 2, '0', ptr);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__MINUTE \
  ptr = put_uint_r((uint64_t)tm.tm_min, 2, '0', ptr);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__SECOND \
  ptr = put_uint_r((uint64_t)tm.tm_sec, 2, '0', ptr);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__MILLISECOND \
  ptr = put_uint_r(msec, 3, '0', ptr);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__PID ptr = put_int_r(pid, 5, ' ', ptr);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__TID ptr = put_int_r(tid, 5, ' ', ptr);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__LEVEL \
  *--ptr = ten_log_level_char(log_msg->level);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__TAG UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__FUNCTION UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__FILENAME UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__FILELINE UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__S(s) PUT_CSTR_R(ptr, s);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__F_INIT(expr)
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R__F_UINT(width, value) \
  ptr = put_uint_r(value, width, ' ', ptr);
#define _TEN_LOG_MESSAGE_FORMAT_PUT_R(field) \
  _PP_CONCAT_3(_TEN_LOG_MESSAGE_FORMAT_PUT_R_, _, field)

#define PUT_CSTR_CHECKED(ptr, end, STR)                               \
  do {                                                                \
    for (size_t i = 0; (end) > (ptr) && (sizeof(STR) - 1) > i; ++i) { \
      *(ptr)++ = (STR)[i];                                            \
    }                                                                 \
  } while (0)

#define PUT_TAG(log_msg, tag, prefix_delim, tag_delim)                         \
  do {                                                                         \
    const char *ch;                                                            \
    log_msg->tag_start = log_msg->buf_content_end;                             \
                                                                               \
    /* Dump tag prefix if specified. */                                        \
    ch = ten_log_tag_prefix;                                                   \
    if (0 != ch) {                                                             \
      for (; log_msg->buf_end != log_msg->buf_content_end &&                   \
             0 != (*log_msg->buf_content_end = *ch);                           \
           ++log_msg->buf_content_end, ++ch) {                                 \
      }                                                                        \
    }                                                                          \
                                                                               \
    ch = tag;                                                                  \
    if (0 != ch && 0 != tag[0]) {                                              \
      if (log_msg->tag_start != log_msg->buf_content_end) {                    \
        /* Dump tag prefix delimeter if specified. */                          \
        PUT_CSTR_CHECKED(log_msg->buf_content_end, log_msg->buf_end,           \
                         prefix_delim);                                        \
      }                                                                        \
      /* Dump tag itself if specified. */                                      \
      for (; log_msg->buf_end != log_msg->buf_content_end &&                   \
             0 != (*log_msg->buf_content_end = *ch);                           \
           ++log_msg->buf_content_end, ++ch) {                                 \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* Dump tag delimeter if specified. */                                     \
    log_msg->tag_end = log_msg->buf_content_end;                               \
    if (log_msg->tag_start != log_msg->buf_content_end) {                      \
      PUT_CSTR_CHECKED(log_msg->buf_content_end, log_msg->buf_end, tag_delim); \
    }                                                                          \
  } while (0)

/**
 * @brief Implements simple put statements for log message specification.
 */
#define _TEN_LOG_MESSAGE_FORMAT_PUT__
#define _TEN_LOG_MESSAGE_FORMAT_PUT__YEAR UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT__MONTH UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT__DAY UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT__HOUR UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT__MINUTE UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT__SECOND UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT__MILLISECOND UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT__PID UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT__TID UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT__LEVEL UNDEFINED
#define _TEN_LOG_MESSAGE_FORMAT_PUT__TAG(prefix_delim, tag_delim) \
  PUT_TAG(log_msg, tag, prefix_delim, tag_delim);
#define _TEN_LOG_MESSAGE_FORMAT_PUT__FUNCTION                            \
  log_msg->buf_content_end =                                             \
      put_string(funcname(src_loc->func_name), log_msg->buf_content_end, \
                 log_msg->buf_end);
#define _TEN_LOG_MESSAGE_FORMAT_PUT__FILENAME                            \
  log_msg->buf_content_end =                                             \
      put_string(filename(src_loc->file_name), log_msg->buf_content_end, \
                 log_msg->buf_end);
#define _TEN_LOG_MESSAGE_FORMAT_PUT__FILELINE \
  log_msg->buf_content_end = put_uint(        \
      src_loc->line, 0, '\0', log_msg->buf_content_end, log_msg->buf_end);
#define _TEN_LOG_MESSAGE_FORMAT_PUT__S(str) \
  PUT_CSTR_CHECKED(log_msg->buf_content_end, log_msg->buf_end, str);
#define _TEN_LOG_MESSAGE_FORMAT_PUT__F_INIT(expr)
#define _TEN_LOG_MESSAGE_FORMAT_PUT__F_UINT(width, value) \
  log_msg->buf_content_end =                              \
      put_uint(value, width, ' ', log_msg->buf_content_end, log_msg->buf_end);
#define _TEN_LOG_MESSAGE_FORMAT_PUT(field) \
  _PP_CONCAT_3(_TEN_LOG_MESSAGE_FORMAT_PUT_, _, field)

/**
 * @brief Expands to expressions that evaluates to true if field is used in
 * specified format specification. Example:
 *
 *   #if _TEN_LOG_MESSAGE_FORMAT_CONTAINS(F_UINT, TEN_LOG_MESSAGE_CTX_FORMAT)
 *       ...
 *   #endif
 */
#define _TEN_LOG_MESSAGE_FORMAT_CONTAINS(field, format) \
  (_TEN_LOG_MESSAGE_FORMAT_MASK(field) & _TEN_LOG_MESSAGE_FORMAT_FIELDS(format))

#define _TEN_LOG_MESSAGE_FORMAT_DATETIME_USED                              \
  (_TEN_LOG_MESSAGE_FORMAT_CONTAINS(YEAR, TEN_LOG_MESSAGE_CTX_FORMAT) ||   \
   _TEN_LOG_MESSAGE_FORMAT_CONTAINS(MONTH, TEN_LOG_MESSAGE_CTX_FORMAT) ||  \
   _TEN_LOG_MESSAGE_FORMAT_CONTAINS(DAY, TEN_LOG_MESSAGE_CTX_FORMAT) ||    \
   _TEN_LOG_MESSAGE_FORMAT_CONTAINS(HOUR, TEN_LOG_MESSAGE_CTX_FORMAT) ||   \
   _TEN_LOG_MESSAGE_FORMAT_CONTAINS(MINUTE, TEN_LOG_MESSAGE_CTX_FORMAT) || \
   _TEN_LOG_MESSAGE_FORMAT_CONTAINS(SECOND, TEN_LOG_MESSAGE_CTX_FORMAT) || \
   _TEN_LOG_MESSAGE_FORMAT_CONTAINS(MILLISECOND, TEN_LOG_MESSAGE_CTX_FORMAT))

/**
 * @brief Default delimiter that separates parts of log message. Can NOT contain
 * '%' or '\0'.
 *
 * Log message format specifications can override (or ignore) this value. For
 * more details see TEN_LOG_MESSAGE_CTX_FORMAT, TEN_LOG_MESSAGE_SRC_FORMAT and
 * TEN_LOG_MESSAGE_TAG_FORMAT.
 */
#ifndef TEN_LOG_DEF_DELIMITER
  #define TEN_LOG_DEF_DELIMITER " "
#endif

/**
 * @brief Specifies log message context format. Log message context includes
 * date, time, process id, thread id and message's log level. Custom information
 * can be added as well. Supported fields: YEAR, MONTH, DAY, HOUR, MINUTE,
 * SECOND, MILLISECOND, PID, TID, LEVEL, S(str), F_INIT(statements),
 * F_UINT(width, value).
 *
 * Must be defined as a tuple, for example:
 *
 *   #define TEN_LOG_MESSAGE_CTX_FORMAT (YEAR, S("."), MONTH, S("."), DAY, S(" >
 * "))
 *
 * In that case, resulting log message will be:
 *
 *   2016.12.22 > TAG function@filename.c:line Message text
 *
 * Note, that tag, source location and message text are not impacted by
 * this setting. See TEN_LOG_MESSAGE_TAG_FORMAT and TEN_LOG_MESSAGE_SRC_FORMAT.
 *
 * If message context must be visually separated from the rest of the message,
 * it must be reflected in context format (notice trailing S(" > ") in the
 * example above).
 *
 * S(str) adds constant string str. String can NOT contain '%' or '\0'.
 *
 * F_INIT(statements) adds initialization statement(s) that will be evaluated
 * once for each log message. All statements are evaluated in specified order.
 * Several F_INIT() fields can be used in every log message format
 * specification. Fields, like F_UINT(width, value), are allowed to use results
 * of initialization statements. If statement introduces variables (or other
 * names, like structures) they must be prefixed with "f_". Statements  must be
 * enclosed into additional "()". Example:
 *
 *   #define TEN_LOG_MESSAGE_CTX_FORMAT \
 *       (F_INIT(( struct rusage f_ru; getrusage(RUSAGE_SELF, &f_ru); )), \
 *        YEAR, S("."), MONTH, S("."), DAY, S(" "), \
 *        F_UINT(5, f_ru.ru_nsignals), \
 *        S(" "))
 *
 * F_UINT(width, value) adds unsigned integer value extended with up to width
 * spaces (for alignment purposes). Value can be any expression that evaluates
 * to unsigned integer. If expression contains non-standard functions, they
 * must be declared with F_INIT(). Example:
 *
 *   #define TEN_LOG_MESSAGE_CTX_FORMAT \
 *        (YEAR, S("."), MONTH, S("."), DAY, S(" "), \
 *        F_INIT(( unsigned tickcount(); )), \
 *        F_UINT(5, tickcount()), \
 *        S(" "))
 *
 * Other log message format specifications follow same rules, but have a
 * different set of supported fields.
 */
#ifndef TEN_LOG_MESSAGE_CTX_FORMAT
  #define TEN_LOG_MESSAGE_CTX_FORMAT                                     \
    (MONTH, S("-"), DAY, S(TEN_LOG_DEF_DELIMITER), HOUR, S(":"), MINUTE, \
     S(":"), SECOND, S("."), MILLISECOND, S(TEN_LOG_DEF_DELIMITER), PID, \
     S(TEN_LOG_DEF_DELIMITER), TID, S(TEN_LOG_DEF_DELIMITER), LEVEL,     \
     S(TEN_LOG_DEF_DELIMITER))
#endif

/**
 * @brief Specifies log message tag format. It includes tag prefix and tag.
 * Custom information can be added as well. Supported fields:
 * TAG(prefix_delimiter, tag_delimiter), S(str), F_INIT(statements),
 * F_UINT(width, value).
 *
 * TAG(prefix_delimiter, tag_delimiter) adds following string to log message:
 *
 *   PREFIX<prefix_delimiter>TAG<tag_delimiter>
 *
 * Prefix delimiter will be used only when prefix is not empty. Tag delimiter
 * will be used only when prefixed tag is not empty. Example:
 *
 *   #define TEN_LOG_MESSAGE_TAG_FORMAT (S("["), TAG(".", ""), S("] "))
 *
 * See TEN_LOG_MESSAGE_CTX_FORMAT for details.
 */
#ifndef TEN_LOG_MESSAGE_TAG_FORMAT
  #define TEN_LOG_MESSAGE_TAG_FORMAT (TAG(".", TEN_LOG_DEF_DELIMITER))
#endif

/**
 * @brief Specifies log message source location format. It includes function
 * name, file name and file line. Custom information can be added as well.
 * Supported fields: FUNCTION, FILENAME, FILELINE, S(str), F_INIT(statements),
 * F_UINT(width, value).
 *
 * See TEN_LOG_MESSAGE_CTX_FORMAT for details.
 */
#ifndef TEN_LOG_MESSAGE_SRC_FORMAT
  #define TEN_LOG_MESSAGE_SRC_FORMAT \
    (FUNCTION, S("@"), FILENAME, S(":"), FILELINE, S(TEN_LOG_DEF_DELIMITER))
#endif
