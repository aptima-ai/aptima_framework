//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>

#include "axis_utils/lib/atomic.h"
#include "axis_utils/sanitizer/thread_check.h"

/**
 * @brief The internal architecture of a shared_ptr/weak_ptr is as follows.
 *
 * shared_ptr \
 * shared_ptr -> ctrl_blk -> data
 *   weak_ptr /
 *
 * A shared_ptr would contribute 1 'shared_cnt' (and additional 1 'weak_cnt' for
 * the 1st shared_ptr), and a weak_ptr would contribute 1 'weak_cnt' only.
 */

typedef enum axis_SMART_PTR_TYPE {
  axis_SMART_PTR_SHARED,
  axis_SMART_PTR_WEAK,
} axis_SMART_PTR_TYPE;

typedef struct axis_smart_ptr_ctrl_blk_t {
  axis_atomic_t shared_cnt;
  axis_atomic_t weak_cnt;

  void *data;  // Points to the shared data.
  void (*destroy)(void *data);
} axis_smart_ptr_ctrl_blk_t;

typedef struct axis_smart_ptr_t {
  axis_signature_t signature;
  axis_sanitizer_thread_check_t thread_check;

  axis_SMART_PTR_TYPE type;

  // Control the life of the memory space pointed by this axis_smart_ptr_t
  // object.
  axis_smart_ptr_ctrl_blk_t *ctrl_blk;
} axis_smart_ptr_t;

typedef axis_smart_ptr_t axis_shared_ptr_t;
typedef axis_smart_ptr_t axis_weak_ptr_t;

typedef bool (*axis_smart_ptr_type_checker)(void *data);

// @{
// Smart pointer

axis_UTILS_PRIVATE_API axis_smart_ptr_t *axis_smart_ptr_clone(
    axis_smart_ptr_t *other);

/**
 * @brief Destroy a smart_ptr.
 */
axis_UTILS_PRIVATE_API void axis_smart_ptr_destroy(axis_smart_ptr_t *self);

/**
 * @brief This function must be used with caution. Essentially, this function
 * can only be used within the TEN runtime and should not be accessed
 * externally. This is because if the parameter is actually a weak_ptr, the
 * function expects the weak_ptr to remain valid after the completion of the
 * function, meaning the object pointed to by the weak_ptr remains valid after
 * the function ends.
 */
axis_UTILS_API void *axis_smart_ptr_get_data(axis_smart_ptr_t *self);

axis_UTILS_API bool axis_smart_ptr_check_type(
    axis_smart_ptr_t *self, axis_smart_ptr_type_checker type_checker);

// @}

// @{
// Shared pointer

#ifdef __cplusplus
#define axis_shared_ptr_create(ptr, destroy) \
  axis_shared_ptr_create_(ptr, reinterpret_cast<void (*)(void *)>(destroy))
#else
#define axis_shared_ptr_create(ptr, destroy) \
  axis_shared_ptr_create_(ptr, (void (*)(void *))(destroy))
#endif

axis_UTILS_API axis_shared_ptr_t *axis_shared_ptr_create_(void *ptr,
                                                       void (*destroy)(void *));

axis_UTILS_API axis_shared_ptr_t *axis_shared_ptr_clone(axis_shared_ptr_t *other);

axis_UTILS_API void axis_shared_ptr_destroy(axis_shared_ptr_t *self);

/**
 * @brief Get the pointing resource.
 */
axis_UTILS_API void *axis_shared_ptr_get_data(axis_shared_ptr_t *self);

// @}

// @{
// Weak pointer

/**
 * @brief Create a weak_ptr from a shared_ptr.
 *
 * @note This function expects that @a shared_ptr is valid.
 */
axis_UTILS_API axis_weak_ptr_t *axis_weak_ptr_create(axis_shared_ptr_t *shared_ptr);

axis_UTILS_API axis_weak_ptr_t *axis_weak_ptr_clone(axis_weak_ptr_t *other);

axis_UTILS_API void axis_weak_ptr_destroy(axis_weak_ptr_t *self);

/**
 * @brief Convert a weak pointer into a shared pointer if the pointing
 * resource is still available.
 */
axis_UTILS_API axis_shared_ptr_t *axis_weak_ptr_lock(axis_weak_ptr_t *self);

// @}
