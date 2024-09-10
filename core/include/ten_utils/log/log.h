//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ten_utils/lib/signature.h"
#include "ten_utils/macro/mark.h"

#define TEN_LOG_SIGNATURE 0x242A93FBC29C297DU

/**
 * @brief "Current" log level is a compile time check and has no runtime
 * overhead. Log level that is below current log level it said to be "disabled".
 * Otherwise, it's "enabled". Log messages that are disabled has no runtime
 * overhead - they are converted to no-op by preprocessor and then eliminated by
 * compiler. Current log level is configured per compilation module (.c/.cpp/.m
 * file) by defining TEN_LOG_DEF_LEVEL or TEN_LOG_LEVEL. TEN_LOG_LEVEL has
 * higher priority and when defined overrides value provided by
 * TEN_LOG_DEF_LEVEL.
 *
 * Common practice is to define default current log level with TEN_LOG_DEF_LEVEL
 * in build script (e.g. Makefile, CMakeLists.txt, gyp, etc.) for the entire
 * project or target:
 *
 *   CC_ARGS := -DTEN_LOG_DEF_LEVEL=TEN_LOG_INFO
 *
 * And when necessary to override it with TEN_LOG_LEVEL in .c/.cpp/.m files
 * before including log/log.h:
 *
 *   #define TEN_LOG_LEVEL TEN_LOG_VERBOSE
 *   #include <log/log.h>
 *
 * If both TEN_LOG_DEF_LEVEL and TEN_LOG_LEVEL are undefined, then TEN_LOG_INFO
 * will be used for release builds (NDEBUG is defined) and TEN_LOG_DEBUG
 * otherwise (NDEBUG is not defined).
 */
#if defined(TEN_LOG_LEVEL)
  #define _TEN_LOG_LEVEL TEN_LOG_LEVEL
#elif defined(TEN_LOG_DEF_LEVEL)
  #define _TEN_LOG_LEVEL TEN_LOG_DEF_LEVEL
#else
  #ifdef NDEBUG
  // Release mode

  // HACK(Wei): We enable DEBUG log in the release mode for now, and to revert
  // it back to INFO in the future if we are confident that the bug in the
  // release mode is clear.
  //
  // #define _TEN_LOG_LEVEL TEN_LOG_INFO
    #define _TEN_LOG_LEVEL TEN_LOG_DEBUG
  #else
  // Debug mode
    #define _TEN_LOG_LEVEL TEN_LOG_DEBUG
  #endif
#endif

/**
 * @brief "Output" log level is a runtime check. When log level is below output
 * log level it said to be "turned off" (or just "off" for short). Otherwise
 * it's "turned on" (or just "on"). Log levels that were "disabled" (see
 * TEN_LOG_LEVEL and TEN_LOG_DEF_LEVEL) can't be "turned on", but "enabled" log
 * levels could be "turned off". Only messages with log level which is
 * "turned on" will reach output facility. All other messages will be ignored
 * (and their arguments will not be evaluated). Output log level is a global
 * property and configured per process using ten_log_set_output_level() function
 * which can be called at any time.
 *
 * Though in some cases it could be useful to configure output log level per
 * compilation module or per library. There are two ways to achieve that:
 * - Define TEN_LOG_OUTPUT_LEVEL to expresion that evaluates to desired output
 *   log level.
 * - Copy log/log.h and src/log/ files into your library and build it with
 *   TEN_LOG_LIBRARY_PREFIX defined to library specific prefix. See
 *   TEN_LOG_LIBRARY_PREFIX for more details.
 *
 * When defined, TEN_LOG_OUTPUT_LEVEL must evaluate to integral value that
 * corresponds to desired output log level. Use it only when compilation module
 * is required to have output log level which is different from global output
 * log level set by ten_log_set_output_level() function. For other cases,
 * consider defining TEN_LOG_LEVEL or using ten_log_set_output_level() function.
 *
 * Example:
 *
 *   #define TEN_LOG_OUTPUT_LEVEL g_module_log_level
 *   #include <log/log.h>
 *   static int g_module_log_level = TEN_LOG_INFO;
 *   static void foo() {
 *       TEN_LOGI("Will check g_module_log_level for output log level");
 *   }
 *   void debug_log(bool on) {
 *       g_module_log_level = on? TEN_LOG_DEBUG: TEN_LOG_INFO;
 *   }
 *
 * Note on performance. This expression will be evaluated each time message is
 * logged (except when message log level is "disabled" - see TEN_LOG_LEVEL for
 * details). Keep this expression as simple as possible, otherwise it will not
 * only add runtime overhead, but also will increase size of call site (which
 * will result in larger executable). The preferred way is to use integer
 * variable (as in example above). If structure must be used, log_level field
 * must be the first field in this structure:
 *
 *   #define TEN_LOG_OUTPUT_LEVEL (g_config.log_level)
 *   #include <log/log.h>
 *   struct config {
 *       int log_level;
 *       unsigned other_field;
 *       [...]
 *   };
 *   static config g_config = {TEN_LOG_INFO, 0, ...};
 *
 * This allows compiler to generate more compact load instruction (no need to
 * specify offset since it's zero). Calling a function to get output log level
 * is generally a bad idea, since it will increase call site size and runtime
 * overhead even further.
 */
#if defined(TEN_LOG_OUTPUT_LEVEL)
  #define _TEN_LOG_OUTPUT_LEVEL TEN_LOG_OUTPUT_LEVEL
#else
  #define _TEN_LOG_OUTPUT_LEVEL ten_log_global_output_level
#endif

/**
 * @brief "Tag" is a compound string that could be associated with a log
 * message. It consists of tag prefix and tag (both are optional).
 *
 * Tag prefix is a global property and configured per process using
 * ten_log_set_tag_prefix() function. Tag prefix identifies context in which
 * component or module is running (e.g. process name). For example, the same
 * library could be used in both client and server processes that work on the
 * same machine. Tag prefix could be used to easily distinguish between them.
 * For more details about tag prefix see ten_log_set_tag_prefix() function.
 *
 * Tag identifies component or module. It is configured per compilation module
 * (.c/.cpp/.m file) by defining TEN_LOG_TAG or TEN_LOG_DEF_TAG. TEN_LOG_TAG has
 * higher priority and when defined overrides value provided by TEN_LOG_DEF_TAG.
 * When defined, value must evaluate to (const char *), so for strings double
 * quotes must be used.
 *
 * Default tag could be defined with TEN_LOG_DEF_TAG in build script (e.g.
 * Makefile, CMakeLists.txt, gyp, etc.) for the entire project or target:
 *
 *   CC_ARGS := -DTEN_LOG_DEF_TAG=\"MISC\"
 *
 * And when necessary could be overriden with TEN_LOG_TAG in .c/.cpp/.m files
 * before including log/log.h:
 *
 *   #define TEN_LOG_TAG "MAIN"
 *   #include <log/log.h>
 *
 * If both TEN_LOG_DEF_TAG and TEN_LOG_TAG are undefined no tag will be added to
 * the log message (tag prefix still could be added though).
 *
 * Output example:
 *
 *   04-29 22:43:20.244 40059  1299 I hello.MAIN Number of arguments: 1
 *                                    |     |
 *                                    |     +- tag (e.g. module)
 *                                    +- tag prefix (e.g. process name)
 */
#if !defined(TEN_LOG_DEF_TAG)
  #define TEN_LOG_DEF_TAG "TEN"
#endif

#if defined(TEN_LOG_TAG)
  #define _TEN_LOG_TAG TEN_LOG_TAG
#elif defined(TEN_LOG_DEF_TAG)
  #define _TEN_LOG_TAG TEN_LOG_DEF_TAG
#else
  #define _TEN_LOG_TAG NULL
#endif

/**
 * @brief Source location format is configured per compilation module
 * (.c/.cpp/.m file) by defining TEN_LOG_DEF_SRC_LOC or TEN_LOG_SRC_LOC.
 * TEN_LOG_SRC_LOC has higher priority and when defined overrides value provided
 * by TEN_LOG_DEF_SRC_LOC.
 *
 * Common practice is to define default format with TEN_LOG_DEF_SRC_LOC in
 * build script (e.g. Makefile, CMakeLists.txt, gyp, etc.) for the entire
 * project or target:
 *
 *   CC_ARGS := -DTEN_LOG_DEF_SRC_LOC=TEN_LOG_SRC_LOC_LONG
 *
 * And when necessary to override it with TEN_LOG_SRC_LOC in .c/.cpp/.m files
 * before including log/log.h:
 *
 *   #define TEN_LOG_SRC_LOC TEN_LOG_SRC_LOC_NONE
 *   #include <log/log.h>
 *
 * If both TEN_LOG_DEF_SRC_LOC and TEN_LOG_SRC_LOC are undefined, then
 * TEN_LOG_SRC_LOC_NONE will be used for release builds (NDEBUG is defined) and
 * TEN_LOG_SRC_LOC_LONG otherwise (NDEBUG is not defined).
 */
#if defined(TEN_LOG_SRC_LOC)
  #define _TEN_LOG_SRC_LOC TEN_LOG_SRC_LOC
#elif defined(TEN_LOG_DEF_SRC_LOC)
  #define _TEN_LOG_SRC_LOC TEN_LOG_DEF_SRC_LOC
#else
  #ifdef NDEBUG
  // Release mode default.

  // HACK(Wei): We enable full location information in the release mode for
  // now, and to revert it back to NONE in the future if we are confident that
  // the bug in the release mode is clear.
  //
  // #define _TEN_LOG_SRC_LOC TEN_LOG_SRC_LOC_NONE
    #define _TEN_LOG_SRC_LOC TEN_LOG_SRC_LOC_LONG
  #else
  // Debug mode default.
    #define _TEN_LOG_SRC_LOC TEN_LOG_SRC_LOC_LONG
  #endif
#endif

#if TEN_LOG_SRC_LOC_LONG == _TEN_LOG_SRC_LOC
  #define _TEN_LOG_SRC_LOC_FUNCTION _TEN_LOG_FUNCTION
#else
  #define _TEN_LOG_SRC_LOC_FUNCTION NULL
#endif

/**
 * @brief Censoring is configured per compilation module (.c/.cpp/.m file) by
 * defining TEN_LOG_DEF_CENSORING or TEN_LOG_CENSORING. TEN_LOG_CENSORING has
 * higher priority and when defined overrides value provided by
 * TEN_LOG_DEF_CENSORING.
 *
 * Common practice is to define default censoring with TEN_LOG_DEF_CENSORING in
 * build script (e.g. Makefile, CMakeLists.txt, gyp, etc.) for the entire
 * project or target:
 *
 *   CC_ARGS := -DTEN_LOG_DEF_CENSORING=TEN_LOG_CENSORED
 *
 * And when necessary to override it with TEN_LOG_CENSORING in .c/.cpp/.m files
 * before including log/log.h (consider doing it only for debug purposes and be
 * very careful not to push such temporary changes to source control):
 *
 *   #define TEN_LOG_CENSORING TEN_LOG_UNCENSORED
 *   #include <log/log.h>
 *
 * If both TEN_LOG_DEF_CENSORING and TEN_LOG_CENSORING are undefined, then
 * TEN_LOG_CENSORED will be used for release builds (NDEBUG is defined) and
 * TEN_LOG_UNCENSORED otherwise (NDEBUG is not defined).
 */
#if defined(TEN_LOG_CENSORING)
  #define _TEN_LOG_CENSORING TEN_LOG_CENSORING
#elif defined(TEN_LOG_DEF_CENSORING)
  #define _TEN_LOG_CENSORING TEN_LOG_DEF_CENSORING
#else
  #ifdef NDEBUG
  // Release mode.
    #define _TEN_LOG_CENSORING TEN_LOG_CENSORED
  #else
  // Debug mode.
    #define _TEN_LOG_CENSORING TEN_LOG_UNCENSORED
  #endif
#endif

/**
 * @brief Check censoring at compile time. Evaluates to true when censoring is
 * disabled (i.e. when secrets will be logged). For example:
 *
 *   #if TEN_LOG_SECRETS
 *       char ssn[16];
 *       getSocialSecurityNumber(ssn);
 *       TEN_LOGI("Customer ssn: %s", ssn);
 *   #endif
 *
 * See TEN_LOG_SECRET() macro for a more convenient way of guarding single log
 * statement.
 */
#define TEN_LOG_SECRETS (_TEN_LOG_CENSORING == TEN_LOG_UNCENSORED)

/**
 * @brief Static (compile-time) initialization support allows to configure
 * logging before entering main() function. This mostly useful in C++ where
 * functions and methods could be called during initialization of global
 * objects. Those functions and methods could record log messages too and for
 * that reason static initialization of logging configuration is customizable.
 *
 * Macros below allow to specify values to use for initial configuration:
 * - TEN_LOG_EXTERN_TAG_PREFIX - tag prefix (default: none)
 * - TEN_LOG_EXTERN_GLOBAL_FORMAT - global format options (default: see
 *   TEN_LOG_MEM_WIDTH)
 * - TEN_LOG_EXTERN_GLOBAL_OUTPUT - global output facility (default: stderr or
 *   platform specific, see TEN_LOG_USE_XXX macros)
 * - TEN_LOG_EXTERN_GLOBAL_OUTPUT_LEVEL - global output log level (default: 0 -
 *   all levels are "turned on")
 *
 * For example, in a file said 'log_config.c':
 *
 *   #include <log/log.h>
 *   TEN_LOG_DEFINE_TAG_PREFIX = "MyApp";
 *   TEN_LOG_DEFINE_GLOBAL_FORMAT = {CUSTOM_MEM_WIDTH};
 *   TEN_LOG_DEFINE_GLOBAL_OUTPUT = {
 *     TEN_LOG_PUT_STD,
 *     custom_output_callback,
 *     0
 *   };
 *   TEN_LOG_DEFINE_GLOBAL_OUTPUT_LEVEL = TEN_LOG_INFO;
 *
 * However, to use any of those macros ten_log library must be compiled with
 * following macros defined:
 * - to use TEN_LOG_DEFINE_TAG_PREFIX define TEN_LOG_EXTERN_TAG_PREFIX
 * - to use TEN_LOG_DEFINE_GLOBAL_FORMAT define TEN_LOG_EXTERN_GLOBAL_FORMAT
 * - to use TEN_LOG_DEFINE_GLOBAL_OUTPUT define TEN_LOG_EXTERN_GLOBAL_OUTPUT
 * - to use TEN_LOG_DEFINE_GLOBAL_OUTPUT_LEVEL define
 *   TEN_LOG_EXTERN_GLOBAL_OUTPUT_LEVEL
 *
 * When ten_log library compiled with one of TEN_LOG_EXTERN_XXX macros defined,
 * corresponding TEN_LOG_DEFINE_XXX macro MUST be used exactly once somewhere.
 * Otherwise build will fail with link error (undefined symbol).
 */
#define TEN_LOG_DEFINE_TAG_PREFIX const char *ten_log_tag_prefix
#define TEN_LOG_DEFINE_GLOBAL_FORMAT ten_log_format_t ten_log_global_format
#define TEN_LOG_DEFINE_GLOBAL_OUTPUT ten_log_output_t ten_log_global_output
#define TEN_LOG_DEFINE_GLOBAL_OUTPUT_LEVEL \
  TEN_LOG_LEVEL ten_log_global_output_level

/**
 * @brief Pointer to global format options. Direct modification is _not_
 * allowed. Use ten_log_set_mem_width() instead. Could be used to initialize
 * ten_log_t structure:
 *
 *   const ten_log_output_t g_output = {TEN_LOG_PUT_STD, output_callback, 0};
 *   const ten_log_t g_spec = {TEN_LOG_SIGNATURE, TEN_LOG_GLOBAL_FORMAT,
 * &g_output};
 *   TEN_LOGI_AUX(&g_spec, "Hello");
 */
#define TEN_LOG_GLOBAL_FORMAT ((ten_log_format_t *)&ten_log_global_format)

/**
 * @brief Pointer to global output variable. Direct modification is _not_
 * allowed. Use ten_log_set_output_v() or ten_log_set_output_p() instead. Could
 * be used to initialize ten_log_t structure:
 *
 *   const ten_log_format_t g_format = {false, 40};
 *   const ten_log_t g_spec = {TEN_LOG_SIGNATURE, g_format,
 * TEN_LOG_GLOBAL_OUTPUT};
 *   TEN_LOGI_AUX(&g_spec, "Hello");
 */
#define TEN_LOG_GLOBAL_OUTPUT ((ten_log_output_t *)&ten_log_global_output)

/**
 * @brief When defined, all library symbols produced by linker will be prefixed
 * with provided value. That allows to use ten_log library privately in another
 * libraries without exposing ten_log symbols in their original form (to avoid
 * possible conflicts with other libraries / components that also could use
 * ten_log for logging). Value must be without quotes, for example:
 *
 *   CC_ARGS := -DTEN_LOG_LIBRARY_PREFIX=my_lib_
 *
 * Note, that in this mode TEN_LOG_LIBRARY_PREFIX must be defined when building
 * ten_log library AND it also must be defined to the same value when building
 * a library that uses it. For example, consider fictional KittyHttp library
 * that wants to use ten_log for logging. First approach that could be taken is
 * to add log/log.h and src/log/ files to the KittyHttp's source code tree
 * directly. In that case it will be enough just to define
 * TEN_LOG_LIBRARY_PREFIX in KittyHttp's build script:
 *
 *   // KittyHttp/CMakeLists.txt
 *   target_compile_definitions(KittyHttp PRIVATE
 *                              "TEN_LOG_LIBRARY_PREFIX=KittyHttp_")
 *
 * If KittyHttp doesn't want to include ten_log source code in its source tree
 * and wants to build ten_log as a separate library than ten_log library must be
 * built with TEN_LOG_LIBRARY_PREFIX defined to KittyHttp_ AND KittyHttp library
 * itself also needs to define TEN_LOG_LIBRARY_PREFIX to KittyHttp_. It can do
 * so either in its build script, as in example above, or by providing a
 * wrapper header that KittyHttp library will need to use instead of log/log.h:
 *
 *   // KittyHttpLogging.h
 *   #define TEN_LOG_LIBRARY_PREFIX KittyHttp_
 *   #include <log/log.h>
 *
 * Regardless of the method chosen, the end result is that ten_log symbols will
 * be prefixed with "KittyHttp_", so if a user of KittyHttp (say DogeBrowser)
 * also uses ten_log for logging, they will not interferer with each other. Both
 * will have their own log level, output facility, format options etc.
 */
#ifdef TEN_LOG_LIBRARY_PREFIX
  #define _TEN_LOG_DECOR__(prefix, name) prefix##name
  #define _TEN_LOG_DECOR_(prefix, name) _TEN_LOG_DECOR__(prefix, name)
  #define _TEN_LOG_DECOR(name) _TEN_LOG_DECOR_(TEN_LOG_LIBRARY_PREFIX, name)

  #define ten_log_set_tag_prefix _TEN_LOG_DECOR(ten_log_set_tag_prefix)
  #define ten_log_set_mem_width _TEN_LOG_DECOR(ten_log_set_mem_width)
  #define ten_log_set_output_level _TEN_LOG_DECOR(ten_log_set_output_level)
  #define ten_log_set_output_v _TEN_LOG_DECOR(ten_log_set_output_v)
  #define ten_log_set_output_p _TEN_LOG_DECOR(ten_log_set_output_p)

  #define ten_log_out_stderr_cb _TEN_LOG_DECOR(ten_log_out_stderr_cb)

  #define ten_log_tag_prefix _TEN_LOG_DECOR(ten_log_tag_prefix)
  #define ten_log_global_format _TEN_LOG_DECOR(ten_log_global_format)
  #define ten_log_global_output _TEN_LOG_DECOR(ten_log_global_output)
  #define ten_log_global_output_level \
    _TEN_LOG_DECOR(ten_log_global_output_level)

  #define ten_log_write_d _TEN_LOG_DECOR(ten_log_write_d)
  #define ten_log_write_aux_d _TEN_LOG_DECOR(ten_log_write_aux_d)
  #define ten_log_write _TEN_LOG_DECOR(ten_log_write)
  #define ten_log_write_aux _TEN_LOG_DECOR(ten_log_write_aux)
  #define ten_log_write_mem_d _TEN_LOG_DECOR(ten_log_write_mem_d)
  #define ten_log_write_mem_aux_d _TEN_LOG_DECOR(ten_log_write_mem_aux_d)
  #define ten_log_write_mem _TEN_LOG_DECOR(ten_log_write_mem)
  #define ten_log_write_mem_aux _TEN_LOG_DECOR(ten_log_write_mem_aux)

  #define ten_log_write_imp _TEN_LOG_DECOR(ten_log_write_imp)
  #define ten_log_stderr_spec _TEN_LOG_DECOR(ten_log_stderr_spec)
#endif

#if defined(__printflike)
  #define _TEN_LOG_PRINTFLIKE(str_index, first_to_check) \
    __printflike(str_index, first_to_check)
#elif defined(__GNUC__)
  #define _TEN_LOG_PRINTFLIKE(str_index, first_to_check) \
    __attribute__((format(__printf__, str_index, first_to_check)))
#else
  #define _TEN_LOG_PRINTFLIKE(str_index, first_to_check)
#endif

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__GNUC__)
  #define _TEN_LOG_FUNCTION __FUNCTION__
#else
  #define _TEN_LOG_FUNCTION __func__
#endif

#if defined(_MSC_VER)
  #define _TEN_LOG_IF(cond)                                             \
    __pragma(warning(push)) __pragma(warning(disable : 4127)) if (cond) \
        __pragma(warning(pop))
  #define _TEN_LOG_WHILE(cond)                                             \
    __pragma(warning(push)) __pragma(warning(disable : 4127)) while (cond) \
        __pragma(warning(pop))
#else
  #define _TEN_LOG_IF(cond) if (cond)
  #define _TEN_LOG_WHILE(cond) while (cond)
#endif

#define _TEN_LOG_NEVER _TEN_LOG_IF(0)
#define _TEN_LOG_ONCE _TEN_LOG_WHILE(0)

/**
 * @brief Execute log statement if condition is true. Example:
 *
 *   TEN_LOG_IF(1 < 2, TEN_LOGI("Log this"));
 *   TEN_LOG_IF(1 > 2, TEN_LOGI("Don't log this"));
 *
 * Keep in mind though, that if condition can't be evaluated at compile time,
 * then it will be evaluated at run time. This will increase executable size
 * and can have noticeable performance overhead. Try to limit conditions to
 * expressions that can be evaluated at compile time.
 */
#define TEN_LOG_IF(cond, f)    \
  do {                         \
    _TEN_LOG_IF((cond)) { f; } \
  } while (0)

/**
 * @brief Mark log statement as "secret". Log statements that are marked as
 * secrets will NOT be executed when censoring is enabled (see
 * TEN_LOG_CENSORED). Example:
 *
 *   TEN_LOG_SECRET(TEN_LOGI("Credit card: %s", credit_card));
 *   TEN_LOG_SECRET(TEN_LOGD_MEM(cipher, cipher_sz, "Cipher bytes:"));
 */
#define TEN_LOG_SECRET(f) TEN_LOG_IF(TEN_LOG_SECRETS, f)

/**
 * @brief Source location is part of a log line that describes location
 * (function or method name, file name and line number, e.g.
 * "runloop@main.cpp:68") of a log statement that produced it.
 *
 * @note Because these values would be used in '#if' statements, it can _not_ be
 * declared as 'enum'.
 */
// Don't add source location to log line.
#define TEN_LOG_SRC_LOC_NONE 0  // NOLINT(modernize-macro-to-enum)
// Add source location in short form (file and line number, e.g.
// "@main.cpp:68").
#define TEN_LOG_SRC_LOC_SHORT 1  // NOLINT(modernize-macro-to-enum)
// Add source location in long form (function or method name, file and line
// number, e.g. "runloop@main.cpp:68").
#define TEN_LOG_SRC_LOC_LONG 2  // NOLINT(modernize-macro-to-enum)

/**
 * @brief Censoring provides conditional logging of secret information, also
 * known as Personally Identifiable Information (PII) or Sensitive Personal
 * Information (SPI). Censoring can be either enabled (TEN_LOG_CENSORED) or
 * disabled (TEN_LOG_UNCENSORED). When censoring is enabled, log statements
 * marked as "secrets" will be ignored and will have zero overhead (arguments
 * also will not be evaluated).
 *
 * @note Because these values would be used in '#if' statements, it can _not_ be
 * declared as 'enum'.
 */
// Censoring is enabled, log statements marked as "secrets" will be ignored
// and will have zero overhead(arguments also will not be evaluated).
#define TEN_LOG_CENSORED 0  // NOLINT(modernize-macro-to-enum)
// Censoring is disabled.
#define TEN_LOG_UNCENSORED 1  // NOLINT(modernize-macro-to-enum)

/**
 * @brief Check "current" log level at compile time (ignoring "output" log
 * level). Evaluates to true when specified log level is enabled. For example:
 *
 *   #if TEN_LOG_ENABLED_DEBUG
 *       const char *const g_enum_strings[] = {
 *           "enum_value_0", "enum_value_1", "enum_value_2"
 *       };
 *   #endif
 *   // ...
 *   #if TEN_LOG_ENABLED_DEBUG
 *       TEN_LOGD("enum value: %s", g_enum_strings[v]);
 *   #endif
 *
 * See TEN_LOG_LEVEL for details.
 */
#define TEN_LOG_ENABLED(level) ((level) >= _TEN_LOG_LEVEL)
#define TEN_LOG_ENABLED_VERBOSE TEN_LOG_ENABLED(TEN_LOG_VERBOSE)
#define TEN_LOG_ENABLED_DEBUG TEN_LOG_ENABLED(TEN_LOG_DEBUG)
#define TEN_LOG_ENABLED_INFO TEN_LOG_ENABLED(TEN_LOG_INFO)
#define TEN_LOG_ENABLED_WARN TEN_LOG_ENABLED(TEN_LOG_WARN)
#define TEN_LOG_ENABLED_ERROR TEN_LOG_ENABLED(TEN_LOG_ERROR)
#define TEN_LOG_ENABLED_FATAL TEN_LOG_ENABLED(TEN_LOG_FATAL)

/**
 * @brief Check "output" log level at run time (taking into account "current"
 * log level as well). Evaluates to true when specified log level is turned on
 * AND enabled. For example:
 *
 *   if (TEN_LOG_ON_DEBUG)
 *   {
 *       char hash[65];
 *       sha256(data_ptr, data_sz, hash);
 *       TEN_LOGD("data: len=%u, sha256=%s", data_sz, hash);
 *   }
 *
 * See TEN_LOG_OUTPUT_LEVEL for details.
 */
#define TEN_LOG_ON(level) \
  (TEN_LOG_ENABLED((level)) && (level) >= _TEN_LOG_OUTPUT_LEVEL)
#define TEN_LOG_ON_VERBOSE TEN_LOG_ON(TEN_LOG_VERBOSE)
#define TEN_LOG_ON_DEBUG TEN_LOG_ON(TEN_LOG_DEBUG)
#define TEN_LOG_ON_INFO TEN_LOG_ON(TEN_LOG_INFO)
#define TEN_LOG_ON_WARN TEN_LOG_ON(TEN_LOG_WARN)
#define TEN_LOG_ON_ERROR TEN_LOG_ON(TEN_LOG_ERROR)
#define TEN_LOG_ON_FATAL TEN_LOG_ON(TEN_LOG_FATAL)

/**
 * @brief Message logging macros:
 * - TEN_LOGV("format string", args, ...)
 * - TEN_LOGD("format string", args, ...)
 * - TEN_LOGI("format string", args, ...)
 * - TEN_LOGW("format string", args, ...)
 * - TEN_LOGE("format string", args, ...)
 * - TEN_LOGF("format string", args, ...)
 *
 * Memory logging macros:
 * - TEN_LOGV_MEM(data_ptr, data_sz, "format string", args, ...)
 * - TEN_LOGD_MEM(data_ptr, data_sz, "format string", args, ...)
 * - TEN_LOGI_MEM(data_ptr, data_sz, "format string", args, ...)
 * - TEN_LOGW_MEM(data_ptr, data_sz, "format string", args, ...)
 * - TEN_LOGE_MEM(data_ptr, data_sz, "format string", args, ...)
 * - TEN_LOGF_MEM(data_ptr, data_sz, "format string", args, ...)
 *
 * Auxiliary logging macros:
 * - TEN_LOGV_AUX(&log_instance, "format string", args, ...)
 * - TEN_LOGD_AUX(&log_instance, "format string", args, ...)
 * - TEN_LOGI_AUX(&log_instance, "format string", args, ...)
 * - TEN_LOGW_AUX(&log_instance, "format string", args, ...)
 * - TEN_LOGE_AUX(&log_instance, "format string", args, ...)
 * - TEN_LOGF_AUX(&log_instance, "format string", args, ...)
 *
 * Auxiliary memory logging macros:
 * - TEN_LOGV_MEM_AUX(&log_instance, data_ptr, data_sz, "format string", args,
 * ...)
 * - TEN_LOGD_MEM_AUX(&log_instance, data_ptr, data_sz, "format string", args,
 * ...)
 * - TEN_LOGI_MEM_AUX(&log_instance, data_ptr, data_sz, "format string", args,
 * ...)
 * - TEN_LOGW_MEM_AUX(&log_instance, data_ptr, data_sz, "format string", args,
 * ...)
 * - TEN_LOGE_MEM_AUX(&log_instance, data_ptr, data_sz, "format string", args,
 * ...)
 * - TEN_LOGF_MEM_AUX(&log_instance, data_ptr, data_sz, "format string", args,
 * ...)
 *
 * Preformatted string logging macros:
 * - TEN_LOGV_STR("preformatted string");
 * - TEN_LOGD_STR("preformatted string");
 * - TEN_LOGI_STR("preformatted string");
 * - TEN_LOGW_STR("preformatted string");
 * - TEN_LOGE_STR("preformatted string");
 * - TEN_LOGF_STR("preformatted string");
 *
 * Explicit log level and tag macros:
 * - TEN_LOG_WRITE(level, tag, "format string", args, ...)
 * - TEN_LOG_WRITE_MEM(level, tag, data_ptr, data_sz, "format string", args,
 * ...)
 * - TEN_LOG_WRITE_AUX(&log_instance, level, tag, "format string", args, ...)
 * - TEN_LOG_WRITE_MEM_AUX(&log_instance, level, tag, data_ptr, data_sz,
 *                        "format string", args, ...)
 *
 * Format string follows printf() conventions. Both data_ptr and data_sz could
 * be 0. Tag can be 0 as well. Most compilers will verify that type of arguments
 * match format specifiers in format string.
 *
 * Library assuming UTF-8 encoding for all strings (char *), including format
 * string itself.
 */
#if TEN_LOG_SRC_LOC_NONE == _TEN_LOG_SRC_LOC

  #define TEN_LOG_WRITE(level, tag, ...)        \
    do {                                        \
      if (TEN_LOG_ON(level)) {                  \
        ten_log_write(level, tag, __VA_ARGS__); \
      }                                         \
    } while (0)

  #define TEN_LOG_WRITE_MEM(level, tag, mem, size, ...)        \
    do {                                                       \
      if (TEN_LOG_ON(level)) {                                 \
        ten_log_write_mem(level, tag, mem, size, __VA_ARGS__); \
      }                                                        \
    } while (0)

  #define TEN_LOG_WRITE_AUX(log, level, tag, ...)        \
    do {                                                 \
      if (TEN_LOG_ON(level)) {                           \
        ten_log_write_aux(log, level, tag, __VA_ARGS__); \
      }                                                  \
    } while (0)

  #define TEN_LOG_WRITE_MEM_AUX(log, level, tag, mem, size, ...)        \
    do {                                                                \
      if (TEN_LOG_ON(level)) {                                          \
        ten_log_write_mem_aux(log, level, tag, mem, size, __VA_ARGS__); \
      }                                                                 \
    } while (0)
#else
  #define TEN_LOG_WRITE(level, tag, ...)                                      \
    do {                                                                      \
      if (TEN_LOG_ON(level)) {                                                \
        ten_log_write_d(_TEN_LOG_SRC_LOC_FUNCTION, __FILE__, __LINE__, level, \
                        tag, __VA_ARGS__);                                    \
      }                                                                       \
    } while (0)

  #define TEN_LOG_WRITE_MEM(level, tag, mem, size, ...)                    \
    do {                                                                   \
      if (TEN_LOG_ON(level)) {                                             \
        ten_log_write_mem_d(_TEN_LOG_SRC_LOC_FUNCTION, __FILE__, __LINE__, \
                            level, tag, mem, size, __VA_ARGS__);           \
      }                                                                    \
    } while (0)

  #define TEN_LOG_WRITE_AUX(log, level, tag, ...)                          \
    do {                                                                   \
      if (TEN_LOG_ON(level)) {                                             \
        ten_log_write_aux_d(_TEN_LOG_SRC_LOC_FUNCTION, __FILE__, __LINE__, \
                            log, level, tag, __VA_ARGS__);                 \
      }                                                                    \
    } while (0)

  #define TEN_LOG_WRITE_MEM_AUX(log, level, tag, mem, size, ...)               \
    do {                                                                       \
      if (TEN_LOG_ON(level)) {                                                 \
        ten_log_write_mem_aux_d(_TEN_LOG_SRC_LOC_FUNCTION, __FILE__, __LINE__, \
                                log, level, tag, mem, size, __VA_ARGS__);      \
      }                                                                        \
    } while (0)
#endif

TEN_UNUSED static void ten_log_unused(const int dummy, ...) { (void)dummy; }

#define TEN_LOG_UNUSED(...)                        \
  do {                                             \
    _TEN_LOG_NEVER ten_log_unused(0, __VA_ARGS__); \
  } while (0)

#if TEN_LOG_ENABLED_VERBOSE
  #define TEN_LOGV(...) \
    TEN_LOG_WRITE(TEN_LOG_VERBOSE, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGV_AUX(log, ...) \
    TEN_LOG_WRITE_AUX(log, TEN_LOG_VERBOSE, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGV_MEM(mem, size, ...) \
    TEN_LOG_WRITE_MEM(TEN_LOG_VERBOSE, _TEN_LOG_TAG, mem, size, __VA_ARGS__)
  #define TEN_LOGV_MEM_AUX(log, mem, size, ...)                      \
    TEN_LOG_WRITE_MEM(log, TEN_LOG_VERBOSE, _TEN_LOG_TAG, mem, size, \
                      __VA_ARGS__)
#else
  #define TEN_LOGV(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGV_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGV_MEM(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGV_MEM_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
#endif

#if TEN_LOG_ENABLED_DEBUG
  #define TEN_LOGD(...) TEN_LOG_WRITE(TEN_LOG_DEBUG, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGD_AUX(log, ...) \
    TEN_LOG_WRITE_AUX(log, TEN_LOG_DEBUG, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGD_MEM(mem, size, ...) \
    TEN_LOG_WRITE_MEM(TEN_LOG_DEBUG, _TEN_LOG_TAG, mem, size, __VA_ARGS__)
  #define TEN_LOGD_MEM_AUX(log, mem, size, ...)                        \
    TEN_LOG_WRITE_MEM_AUX(log, TEN_LOG_DEBUG, _TEN_LOG_TAG, mem, size, \
                          __VA_ARGS__)
#else
  #define TEN_LOGD(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGD_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGD_MEM(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGD_MEM_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
#endif

#if TEN_LOG_ENABLED_INFO
  #define TEN_LOGI(...) TEN_LOG_WRITE(TEN_LOG_INFO, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGI_AUX(log, ...) \
    TEN_LOG_WRITE_AUX(log, TEN_LOG_INFO, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGI_MEM(mem, size, ...) \
    TEN_LOG_WRITE_MEM(TEN_LOG_INFO, _TEN_LOG_TAG, mem, size, __VA_ARGS__)
  #define TEN_LOGI_MEM_AUX(log, mem, size, ...)                       \
    TEN_LOG_WRITE_MEM_AUX(log, TEN_LOG_INFO, _TEN_LOG_TAG, mem, size, \
                          __VA_ARGS__)
#else
  #define TEN_LOGI(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGI_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGI_MEM(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGI_MEM_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
#endif

#if TEN_LOG_ENABLED_WARN
  #define TEN_LOGW(...) TEN_LOG_WRITE(TEN_LOG_WARN, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGW_AUX(log, ...) \
    TEN_LOG_WRITE_AUX(log, TEN_LOG_WARN, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGW_MEM(mem, size, ...) \
    TEN_LOG_WRITE_MEM(TEN_LOG_WARN, _TEN_LOG_TAG, mem, size, __VA_ARGS__)
  #define TEN_LOGW_MEM_AUX(log, mem, size, ...)                       \
    TEN_LOG_WRITE_MEM_AUX(log, TEN_LOG_WARN, _TEN_LOG_TAG, mem, size, \
                          __VA_ARGS__)
#else
  #define TEN_LOGW(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGW_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGW_MEM(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGW_MEM_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
#endif

#if TEN_LOG_ENABLED_ERROR
  #define TEN_LOGE(...) TEN_LOG_WRITE(TEN_LOG_ERROR, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGE_AUX(log, ...) \
    TEN_LOG_WRITE_AUX(log, TEN_LOG_ERROR, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGE_MEM(mem, size, ...) \
    TEN_LOG_WRITE_MEM(TEN_LOG_ERROR, _TEN_LOG_TAG, mem, size, __VA_ARGS__)
  #define TEN_LOGE_MEM_AUX(log, mem, size, ...)                        \
    TEN_LOG_WRITE_MEM_AUX(log, TEN_LOG_ERROR, _TEN_LOG_TAG, mem, size, \
                          __VA_ARGS__)
#else
  #define TEN_LOGE(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGE_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGE_MEM(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGE_MEM_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
#endif

#if TEN_LOG_ENABLED_FATAL
  #define TEN_LOGF(...) TEN_LOG_WRITE(TEN_LOG_FATAL, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGF_AUX(log, ...) \
    TEN_LOG_WRITE_AUX(log, TEN_LOG_FATAL, _TEN_LOG_TAG, __VA_ARGS__)
  #define TEN_LOGF_MEM(mem, size, ...) \
    TEN_LOG_WRITE_MEM(TEN_LOG_FATAL, _TEN_LOG_TAG, mem, size, __VA_ARGS__)
  #define TEN_LOGF_MEM_AUX(log, mem, size, ...)                        \
    TEN_LOG_WRITE_MEM_AUX(log, TEN_LOG_FATAL, _TEN_LOG_TAG, mem, size, \
                          __VA_ARGS__)
#else
  #define TEN_LOGF(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGF_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGF_MEM(...) TEN_LOG_UNUSED(__VA_ARGS__)
  #define TEN_LOGF_MEM_AUX(...) TEN_LOG_UNUSED(__VA_ARGS__)
#endif

#define TEN_LOGV_STR(s) TEN_LOGV("%s", (s))
#define TEN_LOGD_STR(s) TEN_LOGD("%s", (s))
#define TEN_LOGI_STR(s) TEN_LOGI("%s", (s))
#define TEN_LOGW_STR(s) TEN_LOGW("%s", (s))
#define TEN_LOGE_STR(s) TEN_LOGE("%s", (s))
#define TEN_LOGF_STR(s) TEN_LOGF("%s", (s))

/**
 * @brief Predefined spec for stderr. Uses global format options
 * (TEN_LOG_GLOBAL_FORMAT) and TEN_LOG_OUT_STDERR. Could be used to force output
 * to stderr for a particular message. Example:
 *
 *   f = fopen("foo.log", "w");
 *   if (!f) {
 *     TEN_LOGE_AUX(TEN_LOG_STDERR, "Failed to open log file");
 *   }
 */
#define TEN_LOG_STDERR (&ten_log_stderr_spec)

typedef enum TEN_LOG_LEVEL {
  TEN_LOG_NONE,

  TEN_LOG_VERBOSE,
  TEN_LOG_DEBUG,
  TEN_LOG_INFO,
  TEN_LOG_WARN,
  TEN_LOG_ERROR,
  TEN_LOG_FATAL,
} TEN_LOG_LEVEL;

/**
 * @brief Put mask is a set of flags that define what fields will be added to
 * each log message. Default value is TEN_LOG_PUT_STD and other flags could be
 * used to alter its behavior. See ten_log_set_output_v() for more details.
 *
 * Note about TEN_LOG_PUT_SRC: it will be added only in debug builds (NDEBUG is
 * not defined).
 */
typedef enum TEN_LOG_PUT {
  TEN_LOG_PUT_CTX = 1 << 0, /* context (time, pid, tid, log level) */
  TEN_LOG_PUT_TAG = 1 << 1, /* tag (including tag prefix) */
  TEN_LOG_PUT_SRC = 1 << 2, /* source location (file, line, function) */
  TEN_LOG_PUT_MSG = 1 << 3, /* message text (formatted string) */
  TEN_LOG_PUT_STD = 0xffff, /* everything (default) */
} TEN_LOG_PUT;

typedef struct ten_log_message_t {
  TEN_LOG_LEVEL level; /* Log level of the message */
  const char *tag;     /* Associated tag (without tag prefix) */

  char *buf_start; /* Buffer start */
  /* Buffer end (last position where EOL with 0 could be written) */
  char *buf_end;
  char *buf_content_end; /* Buffer content end (append position) */

  char *tag_start; /* Prefixed tag start */
  /* Prefixed tag end (if != tag_start, points to msg separator) */
  char *tag_end;

  char *msg_start; /* Message start (expanded format string) */
} ten_log_message_t;

/**
 * @brief Format options. For more details see ten_log_set_mem_width().
 */
typedef struct ten_log_format_t {
  bool is_allocated;

  size_t mem_width; /* Bytes per line in memory (ASCII-HEX) dump */
} ten_log_format_t;

/**
 * @brief Type of output callback function. It will be called for each log line
 * allowed by both "current" and "output" log levels ("enabled" and "turned
 * on"). Callback function is allowed to modify content of the buffers pointed
 * by the msg, but it's not allowed to modify any of msg fields. Buffer pointed
 * by msg is UTF-8 encoded (no BOM mark).
 */
typedef void (*ten_log_output_func_t)(const ten_log_message_t *msg, void *arg);

typedef void (*ten_log_close_func_t)(void *arg);

/**
 * @brief Output facility.
 */
typedef struct ten_log_output_t {
  bool is_allocated;

  uint64_t mask;  // What to put into log line buffer (see TEN_LOG_PUT_XXX)

  ten_log_output_func_t output_cb;  // Output callback function
  ten_log_close_func_t close_cb;
  void *arg;  // User provided output callback argument
} ten_log_output_t;

/**
 * @brief Used with _AUX macros and allows to override global format and output
 * facility. Use TEN_LOG_GLOBAL_FORMAT and TEN_LOG_GLOBAL_OUTPUT for values from
 * global configuration. Example:
 *
 *   static const ten_log_output_t module_output = {
 *       TEN_LOG_PUT_STD, 0, custom_output_callback
 *   };
 *   static const ten_log_t module_spec = {
 *       TEN_LOG_SIGNATURE,
 *       TEN_LOG_GLOBAL_FORMAT,
 *       &module_output,
 *   };
 *   TEN_LOGI_AUX(&module_spec, "Position: %ix%i", x, y);
 *
 * See TEN_LOGF_AUX and TEN_LOGF_MEM_AUX for details.
 */
typedef struct ten_log_t {
  ten_signature_t signature;

  ten_log_format_t *format;
  ten_log_output_t *output;
} ten_log_t;

#ifdef __cplusplus
extern "C" {
#endif

TEN_UTILS_API const char *ten_log_tag_prefix;
TEN_UTILS_API ten_log_format_t ten_log_global_format;
TEN_UTILS_API ten_log_output_t ten_log_global_output;
TEN_UTILS_API TEN_LOG_LEVEL ten_log_global_output_level;
TEN_UTILS_API const ten_log_t ten_log_stderr_spec;

TEN_UTILS_PRIVATE_API bool ten_log_check_integrity(ten_log_t *self);

TEN_UTILS_API void ten_log_init(ten_log_t *log);

TEN_UTILS_API ten_log_t *ten_log_create(void);

TEN_UTILS_API void ten_log_destroy(ten_log_t *log);

TEN_UTILS_API void ten_log_format_init(ten_log_format_t *format,
                                       size_t mem_width);

TEN_UTILS_API ten_log_format_t *ten_log_format_create(size_t mem_width);

TEN_UTILS_API void ten_log_format_destroy(ten_log_format_t *format);

TEN_UTILS_API void ten_log_output_init(ten_log_output_t *output, uint64_t mask,
                                       ten_log_output_func_t output_cb,
                                       ten_log_close_func_t close_cb,
                                       void *arg);

TEN_UTILS_API ten_log_output_t *ten_log_output_create(
    uint64_t mask, ten_log_output_func_t output_cb,
    ten_log_close_func_t close_cb, void *arg);

TEN_UTILS_API void ten_log_output_destroy(ten_log_output_t *output);

/**
 * @brief Set tag prefix. Prefix will be separated from the tag with dot
 * ('.'). Use NULL or empty string to disable (default). Common use is to
 * set it to the process (or build target) name (e.g. to separate client and
 * server processes). Function will NOT copy provided prefix string, but
 * will store the pointer. Hence specified prefix string must remain valid.
 * See TEN_LOG_DEFINE_TAG_PREFIX for a way to set it before entering main()
 * function. See TEN_LOG_TAG for more information about tag and tag prefix.
 */
TEN_UTILS_API void ten_log_set_tag_prefix(const char *prefix);

/**
 * @brief Set number of bytes per log line in memory (ASCII-HEX) output.
 * Example:
 *
 *   I hello.MAIN 4c6f72656d20697073756d20646f6c6f  Lorem ipsum dolo
 *                |<-          w bytes         ->|  |<-  w chars ->|
 *
 * See TEN_LOGF_MEM and TEN_LOGF_MEM_AUX for more details.
 */
TEN_UTILS_API void ten_log_set_mem_width(size_t width);

/**
 * @brief Set "output" log level. See TEN_LOG_LEVEL and TEN_LOG_OUTPUT_LEVEL for
 * more info about log levels.
 */
TEN_UTILS_API void ten_log_set_output_level(TEN_LOG_LEVEL level);

/**
 * @brief Set output callback function.
 *
 * Mask allows to control what information will be added to the log line buffer
 * before callback function is invoked. Default mask value is TEN_LOG_PUT_STD.
 */
TEN_UTILS_API void ten_log_set_output_v(uint64_t mask,
                                        ten_log_output_func_t output_cb,
                                        ten_log_close_func_t close_cb,
                                        void *arg);
TEN_UNUSED static void ten_log_set_output_p(const ten_log_output_t *output) {
  ten_log_set_output_v(output->mask, output->output_cb, output->close_cb,
                       output->arg);
}

TEN_UTILS_API void ten_log_set_output_to_file(const char *log_path);

TEN_UTILS_API void ten_log_set_output_to_file_aux(ten_log_t *log,
                                                  const char *log_path);

TEN_UTILS_API void ten_log_set_output_to_stderr(void);

TEN_UTILS_API void ten_log_set_output_to_stderr_aux(ten_log_t *log);

TEN_UTILS_API void ten_log_save_output_spec(ten_log_output_t *output);

TEN_UTILS_API void ten_log_restore_output_spec(ten_log_output_t *output);

TEN_UTILS_API void ten_log_close(void);

TEN_UTILS_API void ten_log_close_aux(ten_log_t *self);

TEN_UTILS_API void ten_log_vwrite_d(const char *func, const char *file,
                                    size_t line, TEN_LOG_LEVEL level,
                                    const char *tag, const char *fmt,
                                    va_list va);

TEN_UTILS_API void ten_log_write_d(const char *func, const char *file,
                                   size_t line, TEN_LOG_LEVEL level,
                                   const char *tag, const char *fmt, ...)
    _TEN_LOG_PRINTFLIKE(6, 7);

TEN_UTILS_API void ten_log_write_aux_d(const char *func_name,
                                       const char *file_name, size_t line,
                                       const ten_log_t *log,
                                       TEN_LOG_LEVEL level, const char *tag,
                                       const char *fmt, ...)
    _TEN_LOG_PRINTFLIKE(7, 8);

TEN_UTILS_API void ten_log_write(TEN_LOG_LEVEL level, const char *tag,
                                 const char *fmt, ...)
    _TEN_LOG_PRINTFLIKE(3, 4);

TEN_UTILS_API void ten_log_write_aux(const ten_log_t *log, TEN_LOG_LEVEL level,
                                     const char *tag, const char *fmt, ...)
    _TEN_LOG_PRINTFLIKE(4, 5);

TEN_UTILS_API void ten_log_write_mem_d(const char *func_name,
                                       const char *file_name, size_t line,
                                       TEN_LOG_LEVEL level, const char *tag,
                                       void *buf, size_t buf_size,
                                       const char *fmt, ...)
    _TEN_LOG_PRINTFLIKE(8, 9);

TEN_UTILS_API void ten_log_write_mem_aux_d(const char *func_name,
                                           const char *file_name, size_t line,
                                           ten_log_t *log, TEN_LOG_LEVEL level,
                                           const char *tag, void *buf,
                                           size_t buf_size, const char *fmt,
                                           ...) _TEN_LOG_PRINTFLIKE(9, 10);

TEN_UTILS_API void ten_log_write_mem(TEN_LOG_LEVEL level, const char *tag,
                                     void *buf, size_t buf_size,
                                     const char *fmt, ...)
    _TEN_LOG_PRINTFLIKE(5, 6);

TEN_UTILS_API void ten_log_write_mem_aux(ten_log_t *log, TEN_LOG_LEVEL level,
                                         const char *tag, void *buf,
                                         size_t buf_size, const char *fmt, ...)
    _TEN_LOG_PRINTFLIKE(6, 7);

TEN_UTILS_API const ten_log_t *ten_log_get_global_spec(void);

#ifdef __cplusplus
}
#endif
