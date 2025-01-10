//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "aptima_utils/lib/buf.h"
#include "aptima_utils/lib/signature.h"
#include "aptima_utils/macro/check.h"

#define aptima_STRING_SIGNATURE 0x178445C0402E320DU

#define MAX_BUFFER_SIZE (10 * 1024 * 1024)  // 10 M

#if defined(NDEBUG)
#define aptima_STRING_PRE_BUF_SIZE 256
#define BUFFER_ENLARGE_RATIO 2
#else
// In debug mode, significantly reduce the size of `prebuf` so that
// `aptima_string_reserve` is actually triggered. This way, we can test that even
// if `malloc` occurs within `aptima_string_reserve`, there will be no memory leak.
#define aptima_STRING_PRE_BUF_SIZE 8

// Because the initial buffer size of a string in debug mode is relatively
// small, the enlargement ratio is set higher each time capacity needs to be
// reserved. This helps avoid frequent capacity reservations, which could
// otherwise lead to poor performance.
#define BUFFER_ENLARGE_RATIO 30
#endif

typedef struct aptima_list_t aptima_list_t;

typedef struct aptima_string_t {
  aptima_signature_t signature;

  char *buf;  // Pointer to allocated buffer.
  char pre_buf[aptima_STRING_PRE_BUF_SIZE];
  size_t buf_size;          // Allocated capacity.
  size_t first_unused_idx;  // Index of first unused byte.
} aptima_string_t;

inline bool aptima_string_check_integrity(const aptima_string_t *self) {
  aptima_ASSERT(self, "Invalid argument.");

  if (aptima_signature_get(&self->signature) != aptima_STRING_SIGNATURE) {
    return false;
  }

  // A normal `aptima_string_t`'s `buf` should be a non-NULL value, either pointing
  // to `prebuf` or to memory allocated by `malloc`.
  if (self->buf == NULL) {
    return false;
  }

  return true;
}

/**
 * @brief Create a string object.
 * @return A pointer to the string object.
 */
aptima_UTILS_API aptima_string_t *aptima_string_create(void);

aptima_UTILS_API aptima_string_t *aptima_string_create_from_c_str(const char *str,
                                                         size_t size);

/**
 * @brief Create a string object from c string.
 * @param fmt The c string.
 * @return A pointer to the string object.
 */
aptima_UTILS_API aptima_string_t *aptima_string_create_formatted(const char *fmt, ...);

aptima_UTILS_API aptima_string_t *aptima_string_create_from_va_list(const char *fmt,
                                                           va_list ap);

/**
 * @brief Create a string object from another string object.
 * @param other The other string object.
 * @return A pointer to the string object.
 */
aptima_UTILS_API aptima_string_t *aptima_string_clone(const aptima_string_t *other);

/**
 * @brief Initialize a string object from another string object.
 * @param self The string object.
 * @param other The other string object.
 */
aptima_UTILS_API void aptima_string_copy(aptima_string_t *self, aptima_string_t *other);

/**
 * @brief Initialize a string object from existing memory.
 * @param self The string object.
 */
aptima_UTILS_API void aptima_string_init(aptima_string_t *self);

/**
 * @brief Initialize a string object from existing memory, and set the value.
 * @param self The string object.
 * @param fmt The c string.
 */
aptima_UTILS_API void aptima_string_init_formatted(aptima_string_t *self,
                                             const char *fmt, ...);

aptima_UTILS_API void aptima_string_init_from_string(aptima_string_t *self,
                                               aptima_string_t *other);

/**
 * @brief Initialize a string object from another string object.
 * @param self The string object.
 * @param other The other string object.
 * @param size the max size, copy all if size <= 0
 */
aptima_UTILS_API void aptima_string_init_from_c_str(aptima_string_t *self,
                                              const char *str, size_t size);

/**
 * @brief Destroy a string object and release the memory.
 * @param self The string object.
 */
aptima_UTILS_API void aptima_string_destroy(aptima_string_t *self);

/**
 * @brief Destroy a string object, left the memory.
 * @param self The string object.
 */
aptima_UTILS_API void aptima_string_deinit(aptima_string_t *self);

/**
 * @brief Set the string object as empty.
 * @param self The string object.
 */
aptima_UTILS_API void aptima_string_clear(aptima_string_t *self);

/**
 * @brief Reserve memory for the string object.
 * @param self The string object.
 * @param extra The size of the memory to be reserved.
 */
aptima_UTILS_API void aptima_string_reserve(aptima_string_t *self, size_t extra);

/**
 * @brief Append a c string to the string object.
 * @param self The string object.
 * @param fmt The c string.
 */
aptima_UTILS_API void aptima_string_append_formatted(aptima_string_t *self,
                                               const char *fmt, ...);

aptima_UTILS_API void aptima_string_append_from_va_list(aptima_string_t *self,
                                                  const char *fmt, va_list ap);

/**
 * @brief Prepend a c string to the string object.
 * @param self The string object.
 * @param fmt The c string.
 */
aptima_UTILS_API void aptima_string_prepend_formatted(aptima_string_t *self,
                                                const char *fmt, ...);

aptima_UTILS_API void aptima_string_prepend_from_va_list(aptima_string_t *self,
                                                   const char *fmt, va_list ap);

/**
 * @brief Set the string object with a c string.
 * @param self The string object.
 * @param fmt The c string.
 */
aptima_UTILS_API void aptima_string_set_formatted(aptima_string_t *self, const char *fmt,
                                            ...);

aptima_UTILS_API void aptima_string_set_from_c_str(aptima_string_t *self,
                                             const char *str, size_t size);

/**
 * @brief Check if the string object is empty.
 * @param self The string object.
 * @return true if the string object is empty, otherwise false.
 */
aptima_UTILS_API bool aptima_string_is_empty(const aptima_string_t *self);

/**
 * @brief Check if the string object starts with the specified substring.
 * @param self The string object.
 * @param start The substring to be compared.
 * @return true if the string object starts with the specified substring,
 * otherwise false.
 */
aptima_UTILS_API bool aptima_string_starts_with(const aptima_string_t *self,
                                          const char *start);

/**
 * @brief Check if the string object is equal to another string object.
 * @param self The string object.
 * @param other The other string object.
 * @return true if the string object is equal to the other string object,
 *         otherwise false.
 */
aptima_UTILS_API bool aptima_string_is_equal(const aptima_string_t *self,
                                       const aptima_string_t *other);

/**
 * @brief Check if the string object is equal to a c string.
 * @param self The string object.
 * @param other The c string.
 * @return true if the string object is equal to the c string, otherwise false.
 */
aptima_UTILS_API bool aptima_string_is_equal_c_str(aptima_string_t *self,
                                             const char *other);

/**
 * @brief Check if the string is equal to a c string in case-insensitive flavor.
 * @param self The string object.
 * @param other The c string.
 * @return true if the string object is equal to the c string in
 *         case-insensitive flavor, otherwise false.
 */
aptima_UTILS_API bool aptima_string_is_equal_c_str_case_insensitive(
    aptima_string_t *self, const char *other);

/**
 * @brief Check if the string contains a c string.
 * @param self The string object.
 * @param b The c string.
 * @return true if the string object contains the c string, otherwise false.
 */
aptima_UTILS_API bool aptima_string_contains(aptima_string_t *self, const char *b);

/**
 * @brief Convert the string object to lowercase.
 * @param self The string object.
 */
aptima_UTILS_API void aptima_string_to_lower(aptima_string_t *self);

/**
 * @brief Convert the string object to uppercase.
 * @param self The string object.
 */
aptima_UTILS_API void aptima_string_to_upper(aptima_string_t *self);

/**
 * @brief Get c string from the string object.
 * @param self The string object.
 * @return A pointer to the c string.
 */
inline const char *aptima_string_get_raw_str(const aptima_string_t *self) {
  // It's possible that the return value of this function is used by "%s", and
  // pass NULL as the value of "%s" is an undefined behavior, so we ensure that
  // the return value of this function is not NULL.
  aptima_ASSERT(self && aptima_string_check_integrity(self), "Invalid argument.");
  return self ? self->buf : NULL;
}

/**
 * @brief Get the length of the string object.
 * @param self The string object.
 * @return The length of the string object.
 */
inline size_t aptima_string_len(const aptima_string_t *self) {
  aptima_ASSERT(self && aptima_string_check_integrity(self), "Invalid argument.");
  return self ? self->first_unused_idx : 0;
}

/**
 * @brief Remove @a count characters from the back of the string.
 */
aptima_UTILS_API void aptima_string_erase_back(aptima_string_t *self, size_t count);

/**
 * @brief split string by delimiter.
 * @param self The source string to be splitted.
 * @param delimiter
 * @return the splitted substring list.
 */
aptima_UTILS_API void aptima_string_split(aptima_string_t *self, const char *delimiter,
                                    aptima_list_t *result);

/**
 * @brief Check if the input string is a UUID4 string.
 * @param self The input string.
 * @param result The check result.
 */
aptima_UTILS_API bool aptima_string_is_uuid4(aptima_string_t *self);

/**
 * @brief Convert the buffer content to a hexadecimal string.
 * @param self The string object.
 * @param buf The buffer.
 */
aptima_UTILS_API void aptima_string_hex_from_buf(aptima_string_t *self, aptima_buf_t buf);

aptima_UTILS_API void aptima_string_trim_trailing_slash(aptima_string_t *self);

aptima_UTILS_API void aptima_string_trim_trailing_whitespace(aptima_string_t *self);

aptima_UTILS_API void aptima_string_trim_leading_whitespace(aptima_string_t *self);

aptima_UTILS_API char *aptima_c_string_trim_trailing_whitespace(char *self);

/**
 * @brief Check if the c string is equal to another c string object.
 * @param a The c string object.
 * @param b The other c string object.
 * @return true if the c string a is equal to the other c string b,
 *         otherwise false.
 */
aptima_UTILS_API bool aptima_c_string_is_equal(const char *a, const char *b);

aptima_UTILS_API bool aptima_c_string_is_equal_with_size(const char *a, const char *b,
                                                   size_t num);

/**
 * @brief Check if the c string is equal to a c string in case-insensitive
 * flavor.
 * @param a The c string.
 * @param b The c string.
 * @return true if the c string a is equal to the c string b in case-insensitive
 * flavor, otherwise false.
 */
aptima_UTILS_API bool aptima_c_string_is_equal_case_insensitive(const char *a,
                                                          const char *b);

aptima_UTILS_API bool aptima_c_string_is_equal_with_size_case_insensitive(
    const char *a, const char *b, size_t num);

/**
 * @brief Check if the c string object is empty.
 * @param self The c string object.
 * @return true if the c string object is empty, otherwise false.
 */
aptima_UTILS_API bool aptima_c_string_is_empty(const char *str);

/**
 * @brief Check if the c string starts with another c string.
 * @param self The c string object.
 * @param prefix The prefix c string object.
 * @return true if the c string starts with another c string, otherwise false.
 */
aptima_UTILS_API bool aptima_c_string_starts_with(const char *str,
                                            const char *prefix);

/**
 * @brief Check if the c string ends with another c string.
 * @param self The c string object.
 * @param prefix The postfix c string object.
 * @return true if the c string ends with another c string, otherwise false.
 */
aptima_UTILS_API bool aptima_c_string_ends_with(const char *str, const char *postfix);

/**
 * @brief Check if c string 'a' is smaller than 'b'. The definitions of
 * 'smaller' is as follows.
 *   - The length is smaller.
 *   - If the length is equal, then the first unequal character is smaller.
 */
aptima_UTILS_API bool aptima_c_string_is_equal_or_smaller(const char *a,
                                                    const char *b);

/**
 * @brief find position of a string.
 * @param src
 * @param search string to locate
 * @return the position 'serach' is first found in 'src'; -1 if not found.
 */
aptima_UTILS_API int aptima_c_string_index_of(const char *src, const char *search);

/**
 * @brief split string by delimiter.
 * @param src The source string to be splitted.
 * @param delimiter
 * @return the splitted substring list.
 */
aptima_UTILS_API void aptima_c_string_split(const char *src, const char *delimiter,
                                      aptima_list_t *result);

/**
 * @brief Check if the c string contains a c string.
 * @param self The c string.
 * @param b The c string.
 * @return true if the c string object contains the c string, otherwise false.
 */
aptima_UTILS_API bool aptima_c_string_contains(const char *a, const char *b);

/**
 * @brief Convert a c string to a URI encoded string.
 * @param src The source c string.
 * @param len The length of the source c string.
 * @param result The result string object.
 */
aptima_UTILS_API void aptima_c_string_uri_encode(const char *src, size_t len,
                                           aptima_string_t *result);

/**
 * @brief Convert a c string to a URI decoded string.
 * @param src The source c string.
 * @param len The length of the source c string.
 * @param result The result string object.
 */
aptima_UTILS_API void aptima_c_string_uri_decode(const char *src, size_t len,
                                           aptima_string_t *result);

/**
 * @brief Escape a string by replacing certain special characters by a sequence
 * of an escape character (backslash) and another character and other control
 * characters by a sequence of "\u" followed by a four-digit hex representation.
 * @param src The source string to escape.
 * @param result The output string.
 */
aptima_UTILS_API void aptima_c_string_escaped(const char *src, aptima_string_t *result);

aptima_UTILS_API void aptima_string_slice(aptima_string_t *self, aptima_string_t *other,
                                    char sep);
