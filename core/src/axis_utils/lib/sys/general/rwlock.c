//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/lib/rwlock.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <pthread.h>
#else
// clang-format off
// Stupid Windows doesn't handle header files well
// Include order matters in Windows
#include <windows.h>
#include <synchapi.h>
// clang-format on
#endif

#include "axis_utils/lib/atomic.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/lib/time.h"
#include "axis_utils/macro/macros.h"

#define axis_RWLOCK_SIGNATURE 0xF033C89F0985EB79U

#define axis_YIELD(loop)       \
  do {                        \
    loop++;                   \
    if (loop < 100) {         \
      axis_thread_pause_cpu(); \
    } else if (loop < 1000) { \
      axis_thread_yield();     \
    } else {                  \
      axis_sleep(10);          \
    }                         \
  } while (0)

typedef struct axis_rwlock_op_t {
  int (*init)(axis_rwlock_t *rwlock);
  void (*deinit)(axis_rwlock_t *rwlock);
  int (*lock)(axis_rwlock_t *rwlock, int reader);
  int (*unlock)(axis_rwlock_t *rwlock, int reader);
} axis_rwlock_op_t;

struct axis_rwlock_t {
  axis_signature_t signature;
  axis_rwlock_op_t op;
};

static int axis_rwlock_check_integrity(axis_rwlock_t *self) {
  assert(self);
  if (axis_signature_get(&self->signature) !=
      (axis_signature_t)axis_RWLOCK_SIGNATURE) {
    return 0;
  }
  return 1;
}

static int __axis_rwlock_base_init(axis_rwlock_t *rwlock) {
  assert(rwlock && axis_rwlock_check_integrity(rwlock));
  return 0;
}

static void __axis_rwlock_base_deinit(axis_rwlock_t *rwlock) {
  assert(rwlock && axis_rwlock_check_integrity(rwlock));
}

typedef struct axis_pflock_t {
  axis_rwlock_t base;
  struct {
    axis_atomic_t in;
    axis_atomic_t out;
  } rd, wr;
} axis_pflock_t;

/*
 * Allocation of bits to reader
 *
 * 64                 4 3 2 1 0
 * +-------------------+---+-+-+
 * | rin: reads issued |x|x| | |
 * +-------------------+---+-+-+
 *                          ^ ^
 *                          | |
 * PRES: writer present ----/ |
 * PHID: writer phase id -----/
 *
 * 64                4 3 2 1 0
 * +------------------+------+
 * |rout:read complete|unused|
 * +------------------+------+
 *
 * The maximum number of readers is 2^60 - 1 (more then enough)
 */

/* Constants used to map the bits in reader counter */
#define axis_PFLOCK_WBITS 0x3              /* Writer bits in reader. */
#define axis_PFLOCK_PRES 0x2               /* Writer present bit. */
#define axis_PFLOCK_PHID 0x1               /* Phase ID bit. */
#define axis_PFLOCK_LSB 0xFFFFFFFFFFFFFFF0 /* reader bits. */
#define axis_PFLOCK_RINC 0x10              /* Reader increment. */

static int __axis_pflock_init(axis_rwlock_t *rwlock) {
  assert(rwlock && axis_rwlock_check_integrity(rwlock));

  axis_pflock_t *pflock = (axis_pflock_t *)rwlock;

  pflock->rd.in = 0;
  pflock->rd.out = 0;
  pflock->wr.in = 0;
  pflock->wr.out = 0;

  return 0;
}

static void __axis_pflock_deinit(axis_rwlock_t *rwlock) {
  assert(rwlock && axis_rwlock_check_integrity(rwlock));

  // do nothing!
}

static int __axis_pflock_lock(axis_rwlock_t *rwlock, int reader) {
  assert(rwlock && axis_rwlock_check_integrity(rwlock));

  axis_pflock_t *pflock = (axis_pflock_t *)rwlock;
  int loops = 0;
  if (reader) {
    uint64_t w;

    /*
     * If no writer is present, then the operation has completed
     * successfully.
     */
    w = axis_atomic_fetch_add(&pflock->rd.in, axis_PFLOCK_RINC) &
        axis_PFLOCK_WBITS;
    if (w == 0) {
      return 0;
    }

    /* Wait for current write phase to complete. */
    loops = 0;
    while ((axis_atomic_load(&pflock->rd.in) & axis_PFLOCK_WBITS) == w) {
      axis_YIELD(loops);
    }
  } else {
    uint64_t ticket, w;

    /* Acquire ownership of write-phase.
     * This is same as axis_tickelock_lock().
     */
    ticket = axis_atomic_fetch_add(&pflock->wr.in, 1);

    loops = 0;
    while (axis_atomic_load(&pflock->wr.out) != ticket) {
      axis_YIELD(loops);
    }

    /*
     * Acquire ticket on read-side in order to allow them
     * to flush. Indicates to any incoming reader that a
     * write-phase is pending.
     *
     * The load of rd.out in wait loop could be executed
     * speculatively.
     */
    w = axis_PFLOCK_PRES | (ticket & axis_PFLOCK_PHID);
    ticket = axis_atomic_fetch_add(&pflock->rd.in, w);

    /* Wait for any pending readers to flush. */
    loops = 0;
    while (axis_atomic_load(&pflock->rd.out) != ticket) {
      axis_YIELD(loops);
    }
  }

  return 0;
}

static int __axis_pflock_unlock(axis_rwlock_t *rwlock, int reader) {
  assert(rwlock && axis_rwlock_check_integrity(rwlock));

  axis_pflock_t *pflock = (axis_pflock_t *)rwlock;

  if (reader) {
    axis_atomic_fetch_add(&pflock->rd.out, axis_PFLOCK_RINC);
  } else {
    /* Migrate from write phase to read phase. */
    axis_atomic_fetch_and(&pflock->rd.in, axis_PFLOCK_LSB);

    /* Allow other writers to continue. */
    axis_atomic_fetch_add(&pflock->wr.out, 1);
  }

  return 0;
}

static inline axis_rwlock_t *__axis_pflock_create() {
  axis_pflock_t *pflock = (axis_pflock_t *)malloc(sizeof(axis_pflock_t));
  if (pflock == NULL) {
    return NULL;
  }

  memset(pflock, 0, sizeof(axis_pflock_t));
  axis_signature_set(&pflock->base.signature, axis_RWLOCK_SIGNATURE);
  pflock->base.op.init = __axis_pflock_init;
  pflock->base.op.deinit = __axis_pflock_deinit;
  pflock->base.op.lock = __axis_pflock_lock;
  pflock->base.op.unlock = __axis_pflock_unlock;
  return &pflock->base;
}

typedef struct axis_native_t {
  axis_rwlock_t base;
#if defined(_WIN32)
  SRWLOCK native;
#else
  pthread_rwlock_t native;
#endif
} axis_native_t;

#if defined(_WIN32)

static int __axis_native_init(axis_rwlock_t *rwlock) {
  axis_native_t *native = (axis_native_t *)rwlock;
  InitializeSRWLock(&native->native);
  return 0;
}

static void __axis_native_deinit(axis_rwlock_t *rwlock) {}

static int __axis_native_lock(axis_rwlock_t *rwlock, int reader) {
  axis_native_t *native = (axis_native_t *)rwlock;
  if (reader) {
    AcquireSRWLockShared(&native->native);
  } else {
    AcquireSRWLockExclusive(&native->native);
  }
  return 0;
}

static int __axis_native_unlock(axis_rwlock_t *rwlock, int reader) {
  axis_native_t *native = (axis_native_t *)rwlock;
  if (reader) {
    ReleaseSRWLockShared(&native->native);
  } else {
    ReleaseSRWLockExclusive(&native->native);
  }
  return 0;
}

#else

static int __axis_native_init(axis_rwlock_t *rwlock) {
  assert(rwlock && axis_rwlock_check_integrity(rwlock));

  axis_native_t *native = (axis_native_t *)rwlock;
  return pthread_rwlock_init(&native->native, NULL);
}

static void __axis_native_deinit(axis_rwlock_t *rwlock) {
  assert(rwlock && axis_rwlock_check_integrity(rwlock));

  axis_native_t *native = (axis_native_t *)rwlock;
  pthread_rwlock_destroy(&native->native);
}

static int __axis_native_lock(axis_rwlock_t *rwlock, int reader) {
  assert(rwlock && axis_rwlock_check_integrity(rwlock));

  axis_native_t *native = (axis_native_t *)rwlock;
  return reader ? pthread_rwlock_rdlock(&native->native)
                : pthread_rwlock_wrlock(&native->native);
}

static int __axis_native_unlock(axis_rwlock_t *rwlock, int reader) {
  assert(rwlock && axis_rwlock_check_integrity(rwlock));

  axis_native_t *native = (axis_native_t *)rwlock;
  return pthread_rwlock_unlock(&native->native);
}

#endif

static inline axis_rwlock_t *__axis_native_create() {
  axis_native_t *native = (axis_native_t *)malloc(sizeof(axis_native_t));
  if (native == NULL) {
    return NULL;
  }

  memset(native, 0, sizeof(axis_native_t));
  axis_signature_set(&native->base.signature, axis_RWLOCK_SIGNATURE);
  native->base.op.init = __axis_native_init;
  native->base.op.deinit = __axis_native_deinit;
  native->base.op.lock = __axis_native_lock;
  native->base.op.unlock = __axis_native_unlock;
  return &native->base;
}

axis_rwlock_t *axis_rwlock_create(axis_RW_FAIRNESS fair) {
  axis_rwlock_t *rwlock = NULL;
  switch (fair) {
    case axis_RW_PHASE_FAIR:
      rwlock = __axis_pflock_create();
      break;
    case axis_RW_NATIVE:
      rwlock = __axis_native_create();
      break;
    default:
      break;
  }

  if (UNLIKELY(!rwlock)) {
    return NULL;
  }

  assert(rwlock && axis_rwlock_check_integrity(rwlock));

  if (__axis_rwlock_base_init(rwlock) != 0) {
    free(rwlock);
    return NULL;
  }

  if (LIKELY(rwlock->op.init != NULL)) {
    if (rwlock->op.init(rwlock) != 0) {
      axis_rwlock_destroy(rwlock);
      return NULL;
    }
  }

  return rwlock;
}

void axis_rwlock_destroy(axis_rwlock_t *lock) {
  if (UNLIKELY(!lock)) {
    return;
  }

  assert(lock && axis_rwlock_check_integrity(lock));

  if (LIKELY(lock->op.deinit != NULL)) {
    lock->op.deinit(lock);
  }

  __axis_rwlock_base_deinit(lock);

  free(lock);
}

int axis_rwlock_lock(axis_rwlock_t *lock, int reader) {
  if (UNLIKELY(!lock)) {
    return -1;
  }

  assert(lock && axis_rwlock_check_integrity(lock));

  if (LIKELY(lock->op.lock != NULL)) {
    return lock->op.lock(lock, reader);
  }

  return -1;
}

int axis_rwlock_unlock(axis_rwlock_t *lock, int reader) {
  if (UNLIKELY(!lock)) {
    return -1;
  }

  assert(lock && axis_rwlock_check_integrity(lock));

  if (LIKELY(lock->op.unlock != NULL)) {
    return lock->op.unlock(lock, reader);
  }

  return -1;
}
