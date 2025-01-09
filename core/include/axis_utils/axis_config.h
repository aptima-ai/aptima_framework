//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#ifdef __cplusplus
#define aptima_EXTERN_C extern "C"
#else
#define aptima_EXTERN_C extern
#endif

#if defined(_WIN32)

#if defined(aptima_UTILS_EXPORT)
#if !defined(aptima_UTILS_API)
#define aptima_UTILS_API aptima_EXTERN_C __declspec(dllexport)
#endif
#else
#if !defined(aptima_UTILS_API)
#define aptima_UTILS_API aptima_EXTERN_C __declspec(dllimport)
#endif
#endif

#if !defined(aptima_UTILS_PRIVATE_API)
#define aptima_UTILS_PRIVATE_API aptima_EXTERN_C
#endif

#elif defined(__APPLE__)

#include <TargetConditionals.h>

#if !defined(aptima_UTILS_API)
#define aptima_UTILS_API __attribute__((visibility("default"))) aptima_EXTERN_C
#endif

#if !defined(aptima_UTILS_PRIVATE_API)
#define aptima_UTILS_PRIVATE_API __attribute__((visibility("hidden"))) aptima_EXTERN_C
#endif

#elif defined(__linux__)

#if !defined(aptima_UTILS_API)
#define aptima_UTILS_API aptima_EXTERN_C __attribute__((visibility("default")))
#endif

#if !defined(aptima_UTILS_PRIVATE_API)
#define aptima_UTILS_PRIVATE_API aptima_EXTERN_C __attribute__((visibility("hidden")))
#endif

#else

#if !defined(aptima_UTILS_API)
#define aptima_UTILS_API aptima_EXTERN_C
#endif

#if !defined(aptima_UTILS_PRIVATE_API)
#define aptima_UTILS_PRIVATE_API aptima_EXTERN_C
#endif

#endif

#include <stdint.h>

#if !defined(ssize_t)
typedef intptr_t ssize_t;
#endif
