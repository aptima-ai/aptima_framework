//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#if defined(__GNUC__) || defined(__clang__)
#ifndef aptima_CONSTRUCTOR
#define aptima_CONSTRUCTOR(name) \
  __attribute__((constructor, used)) static void name(void)
#endif

#ifndef aptima_DESTRUCTOR
#define aptima_DESTRUCTOR(name) \
  __attribute__((destructor, used)) static void name(void)
#endif

#else  // defined(__GNUC__) || defined(__clang__)

#if defined(__cplusplus)

#ifndef aptima_CONSTRUCTOR
#define aptima_CONSTRUCTOR(name)                                             \
  static void name(void);                                                 \
  struct ____dummy__ctor_##name##__struct {                               \
    ____dummy__ctor_##name##__struct() { name(); }                        \
    ~____dummy__ctor_##name##__struct() {}                                \
  };                                                                      \
  static const ____dummy__ctor_##name##__struct ____dummy_ctor_of_##name; \
  static void name(void)
#endif

#ifndef aptima_DESTRUCTOR
#define aptima_DESTRUCTOR(name)                                              \
  static void name(void);                                                 \
  struct ____dummy__dtor_##name##__struct {                               \
    ____dummy__dtor_##name##__struct() {}                                 \
    ~____dummy__dtor_##name##__struct() { name(); }                       \
  };                                                                      \
  static const ____dummy__dtor_##name##__struct ____dummy_dtor_of_##name; \
  static void name(void)
#endif

#else  // defined(__cplusplus)

/**
 * Put it in user-defined global initializers, which msvc name it ".CRT$XCU".
 * This section have to be 'read' property otherwise compiler complains.
 * See this link[1] for more information.
 * Also we should be careful about linker optimize that may wipe our symbol
 * out, just like link[1] said. [1]:
 * https://docs.microsoft.com/en-us/cpp/c-runtime-library/crt-initialization
 *
 */

#pragma section(".CRT$XCU", read)

#ifndef AGORE_aptima_CONSTRUCTOR__
#define AGORE_aptima_CONSTRUCTOR__(name, prefix)                                 \
  static void name(void);                                                     \
  __declspec(allocate(".CRT$XCU")) void (*____pfnDummy##name##)(void) = name; \
  __pragma(comment(linker, "/include:" prefix "____pfnDummy" #name));         \
  static void name(void)
#endif

#if defined(_WIN64)
#ifndef aptima_CONSTRUCTOR
#define aptima_CONSTRUCTOR(name) AGORE_aptima_CONSTRUCTOR__(name, "")
#endif
#else  // defined(_WIN64)
#ifndef aptima_CONSTRUCTOR
#define aptima_CONSTRUCTOR(name) AGORE_aptima_CONSTRUCTOR__(name, "_")
#endif
#endif  // defined(_WIN64)

#endif  // defined(__cplusplus)

// No destructor for you, sorry.
// Function marked as destructor will _not_ be called in Windows
#ifndef aptima_DESTRUCTOR
#define aptima_DESTRUCTOR(name) static void name(void)
#endif

#endif  // defined(__GNUC__)