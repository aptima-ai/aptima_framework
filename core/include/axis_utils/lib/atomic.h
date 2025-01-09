//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

#include <stdint.h>

typedef int64_t aptima_atomic_t;

/**
 * @brief Load from an atomic variable.
 * @param a The pointer to the atomic variable.
 * @return The value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_load(volatile aptima_atomic_t *a);

/**
 * @brief Store to an atomic variable.
 * @param a The pointer to the atomic variable.
 * @param v The value to store.
 */
aptima_UTILS_API void aptima_atomic_store(volatile aptima_atomic_t *a, int64_t v);

/**
 * @brief Add to an atomic variable, and return the old value.
 * @param a The pointer to the atomic variable.
 * @param v The value to add.
 * @return The original value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_fetch_add(volatile aptima_atomic_t *a, int64_t v);

/**
 * @brief Add to an atomic variable, and return the new value.
 * @param a The pointer to the atomic variable.
 * @param v The value to add.
 * @return The new value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_add_fetch(volatile aptima_atomic_t *a, int64_t v);

/**
 * @brief And to an atomic variable, and return the new value.
 * @param a The pointer to the atomic variable.
 * @param v The value to and.
 * @return The new value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_and_fetch(volatile aptima_atomic_t *a, int64_t v);

/**
 * @brief Subtract from an atomic variable, and return the old value.
 * @param a The pointer to the atomic variable.
 * @param v The value to subtract.
 * @return The original value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_fetch_sub(volatile aptima_atomic_t *a, int64_t v);

/**
 * @brief Subtract from an atomic variable, and return the new value.
 * @param a The pointer to the atomic variable.
 * @param v The value to subtract.
 * @return The new value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_sub_fetch(volatile aptima_atomic_t *a, int64_t v);

/**
 * @brief OR from an atomic variable, and return the new value.
 * @param a The pointer to the atomic variable.
 * @param v The value to OR.
 * @return The new value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_or_fetch(volatile aptima_atomic_t *a, int64_t v);

/**
 * @brief Binary and to an atomic variable, and return the old value.
 * @param a The pointer to the atomic variable.
 * @param v The value to perform and operation.
 * @return The old value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_fetch_and(volatile aptima_atomic_t *a, int64_t v);

/**
 * @brief Binary or to an atomic variable, and return the old value.
 * @param a The pointer to the atomic variable.
 * @param v The value to perform or operation.
 * @return The old value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_fetch_or(volatile aptima_atomic_t *a, int64_t v);

/**
 * @brief Set an atomic variable to a value and return the old value.
 * @param a The pointer to the atomic variable.
 * @param v The value to set.
 * @return The original value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_test_set(volatile aptima_atomic_t *a, int64_t v);

/**
 * @brief Compare and exchange an atomic variable. Returns the origin value.
 * @param a The pointer to the atomic variable.
 * @param comp The expected value.
 * @param xchg The new value.
 * @return The original value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_val_compare_swap(volatile aptima_atomic_t *a,
                                                  int64_t comp, int64_t xchg);

/**
 * @brief Compare and exchange an atomic variable with a value.
 *        Returns the compare result of origin value and |comp|.
 * @param a The pointer to the atomic variable.
 * @param comp The expected value.
 * @param xchg The new value.
 * @return true if original value equals |comp|, false otherwise.
 */
aptima_UTILS_API int aptima_atomic_bool_compare_swap(volatile aptima_atomic_t *a,
                                               int64_t comp, int64_t xchg);

/**
 * @brief Increment an atomic variable if its value is not zero.
 * @param a The pointer to the atomic variable.
 * @return The original value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_inc_if_non_zero(volatile aptima_atomic_t *a);

/**
 * @brief Decrement an atomic variable if its value is not zero.
 * @param a The pointer to the atomic variable.
 * @return The original value of the atomic variable.
 */
aptima_UTILS_API int64_t aptima_atomic_dec_if_non_zero(volatile aptima_atomic_t *a);

/**
 * @brief Put a full memory barrier
 */
aptima_UTILS_API void aptima_memory_barrier();

#if defined(_WIN32)
#include <intrin.h>
#define aptima_compiler_barrier() _ReadWriteBarrier()
#else
#define aptima_compiler_barrier()       \
  do {                               \
    asm volatile("" : : : "memory"); \
  } while (0)
#endif
