//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdbool.h>

#include "aptima_utils/lib/atomic.h"
#include "aptima_utils/sanitizer/thread_check.h"

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

typedef enum aptima_SMART_PTR_TYPE {
  aptima_SMART_PTR_SHARED,
  aptima_SMART_PTR_WEAK,
} aptima_SMART_PTR_TYPE;

typedef struct aptima_smart_ptr_ctrl_blk_t {
  aptima_atomic_t shared_cnt;
  aptima_atomic_t weak_cnt;

  void *data;  // Points to the shared data.
  void (*destroy)(void *data);
} aptima_smart_ptr_ctrl_blk_t;

typedef struct aptima_smart_ptr_t {
  aptima_signature_t signature;
  aptima_sanitizer_thread_check_t thread_check;

  aptima_SMART_PTR_TYPE type;

  // Control the life of the memory space pointed by this aptima_smart_ptr_t
  // object.
  aptima_smart_ptr_ctrl_blk_t *ctrl_blk;
} aptima_smart_ptr_t;

typedef aptima_smart_ptr_t aptima_shared_ptr_t;
typedef aptima_smart_ptr_t aptima_weak_ptr_t;

typedef bool (*aptima_smart_ptr_type_checker)(void *data);

// @{
// Smart pointer

aptima_UTILS_PRIVATE_API aptima_smart_ptr_t *aptima_smart_ptr_clone(
    aptima_smart_ptr_t *other);

/**
 * @brief Destroy a smart_ptr.
 */
aptima_UTILS_PRIVATE_API void aptima_smart_ptr_destroy(aptima_smart_ptr_t *self);

/**
 * @brief This function must be used with caution. Essentially, this function
 * can only be used within the APTIMA runtime and should not be accessed
 * externally. This is because if the parameter is actually a weak_ptr, the
 * function expects the weak_ptr to remain valid after the completion of the
 * function, meaning the object pointed to by the weak_ptr remains valid after
 * the function ends.
 */
aptima_UTILS_API void *aptima_smart_ptr_get_data(aptima_smart_ptr_t *self);

aptima_UTILS_API bool aptima_smart_ptr_check_type(
    aptima_smart_ptr_t *self, aptima_smart_ptr_type_checker type_checker);

// @}

// @{
// Shared pointer

#ifdef __cplusplus
#define aptima_shared_ptr_create(ptr, destroy) \
  aptima_shared_ptr_create_(ptr, reinterpret_cast<void (*)(void *)>(destroy))
#else
#define aptima_shared_ptr_create(ptr, destroy) \
  aptima_shared_ptr_create_(ptr, (void (*)(void *))(destroy))
#endif

aptima_UTILS_API aptima_shared_ptr_t *aptima_shared_ptr_create_(void *ptr,
                                                       void (*destroy)(void *));

aptima_UTILS_API aptima_shared_ptr_t *aptima_shared_ptr_clone(aptima_shared_ptr_t *other);

aptima_UTILS_API void aptima_shared_ptr_destroy(aptima_shared_ptr_t *self);

/**
 * @brief Get the pointing resource.
 */
aptima_UTILS_API void *aptima_shared_ptr_get_data(aptima_shared_ptr_t *self);

// @}

// @{
// Weak pointer

/**
 * @brief Create a weak_ptr from a shared_ptr.
 *
 * @note This function expects that @a shared_ptr is valid.
 */
aptima_UTILS_API aptima_weak_ptr_t *aptima_weak_ptr_create(aptima_shared_ptr_t *shared_ptr);

aptima_UTILS_API aptima_weak_ptr_t *aptima_weak_ptr_clone(aptima_weak_ptr_t *other);

aptima_UTILS_API void aptima_weak_ptr_destroy(aptima_weak_ptr_t *self);

/**
 * @brief Convert a weak pointer into a shared pointer if the pointing
 * resource is still available.
 */
aptima_UTILS_API aptima_shared_ptr_t *aptima_weak_ptr_lock(aptima_weak_ptr_t *self);

// @}
