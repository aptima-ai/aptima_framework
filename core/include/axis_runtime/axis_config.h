//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#ifdef __cplusplus
#define axis_EXTERN_C extern "C"
#else
#define axis_EXTERN_C extern
#endif

#if defined(_WIN32)

#if defined(axis_RUNTIME_EXPORT)
#if !defined(axis_RUNTIME_API)
#define axis_RUNTIME_API axis_EXTERN_C __declspec(dllexport)
#endif
#else
#if !defined(axis_RUNTIME_API)
#define axis_RUNTIME_API axis_EXTERN_C __declspec(dllimport)
#endif
#endif

#if !defined(axis_RUNTIME_PRIVATE_API)
#define axis_RUNTIME_PRIVATE_API axis_EXTERN_C
#endif

#elif defined(__APPLE__)

#include <TargetConditionals.h>

#if !defined(axis_RUNTIME_API)
#define axis_RUNTIME_API __attribute__((visibility("default"))) axis_EXTERN_C
#endif

#if !defined(axis_RUNTIME_PRIVATE_API)
#define axis_RUNTIME_PRIVATE_API \
  __attribute__((visibility("hidden"))) axis_EXTERN_C
#endif

#elif defined(__linux__)

#if !defined(axis_RUNTIME_API)
#define axis_RUNTIME_API axis_EXTERN_C __attribute__((visibility("default")))
#endif

#if !defined(axis_RUNTIME_PRIVATE_API)
#define axis_RUNTIME_PRIVATE_API \
  axis_EXTERN_C __attribute__((visibility("hidden")))
#endif

#else

#if !defined(axis_RUNTIME_API)
#define axis_RUNTIME_API axis_EXTERN_C
#endif

#if !defined(axis_RUNTIME_PRIVATE_API)
#define axis_RUNTIME_PRIVATE_API axis_EXTERN_C
#endif

#endif
