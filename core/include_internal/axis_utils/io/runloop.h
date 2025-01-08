//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/lib/signature.h"
#include "axis_utils/sanitizer/thread_check.h"

#define axis_RUNLOOP_SIGNATURE 0x9B51152AD12240ADU
#define axis_RUNLOOP_ASYNC_SIGNATURE 0x7A8A95F63EBDC10EU
#define axis_RUNLOOP_TIMER_SIGNATURE 0xC20D3F27E11BE93CU

/**
 * @brief The base struct of the runloop.
 */
typedef struct axis_runloop_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  /**
   * @brief The name of the type of the runloop.
   */
  const char *impl;

  /**
   * @brief Points to the user-defined memory space.
   */
  void *data;
} axis_runloop_t;

/**
 * @brief The base struct of the asynchronous triggering mechanism of the
 * runloop.
 */
typedef struct axis_runloop_async_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  /**
   * @brief The name of the type of the runloop.
   */
  const char *impl;

  /**
   * @brief The attached runloop.
   */
  axis_runloop_t *loop;

  /**
   * @brief Points to the user-defined memory space.
   */
  void *data;
} axis_runloop_async_t;

typedef struct axis_runloop_timer_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  /**
   * @brief The name of the type of the runloop.
   */
  const char *impl;

  /**
   * @brief The attached runloop.
   */
  axis_runloop_t *loop;

  uint64_t timeout;
  uint64_t periodic;

  /**
   * @brief Points to the user-defined memory space.
   */
  void *data;
} axis_runloop_timer_t;

typedef enum axis_RUNLOOP_STATE {
  axis_RUNLOOP_STATE_IDLE,
  axis_RUNLOOP_STATE_RUNNING,
} axis_RUNLOOP_STATE;

axis_UTILS_PRIVATE_API bool axis_runloop_async_check_integrity(
    axis_runloop_async_t *self, bool check_thread);

axis_UTILS_PRIVATE_API bool axis_runloop_timer_check_integrity(
    axis_runloop_timer_t *self, bool check_thread);
