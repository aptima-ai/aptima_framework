//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_utils/lib/rwlock.h"

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

#include "ten_utils/lib/atomic.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/lib/time.h"
#include "ten_utils/macro/macros.h"

#define TEN_RWLOCK_SIGNATURE 0xF033C89F0985EB79U

#define TEN_YIELD(loop)       \
  do {                        \
    loop++;                   \
    if (loop < 100) {         \
      ten_thread_pause_cpu(); \
    } else if (loop < 1000) { \
      ten_thread_yield();     \
    } else {                  \
      ten_sleep(10);          \
    }                         \
  } while (0)

typedef struct ten_rwlock_op_t {
  int (*init)(ten_rwlock_t *rwlock);
  void (*deinit)(ten_rwlock_t *rwlock);
  int (*lock)(ten_rwlock_t *rwlock, int reader);
  int (*unlock)(ten_rwlock_t *rwlock, int reader);
} ten_rwlock_op_t;

struct ten_rwlock_t {
  ten_signature_t signature;
  ten_rwlock_op_t op;
};

static int ten_rwlock_check_integrity(ten_rwlock_t *self) {
  assert(self);
  if (ten_signature_get(&self->signature) !=
      (ten_signature_t)TEN_RWLOCK_SIGNATURE) {
    return 0;
  }
  return 1;
}

static int __ten_rwlock_base_init(ten_rwlock_t *rwlock) {
  assert(rwlock && ten_rwlock_check_integrity(rwlock));
  return 0;
}

static void __ten_rwlock_base_deinit(ten_rwlock_t *rwlock) {
  assert(rwlock && ten_rwlock_check_integrity(rwlock));
}

typedef struct ten_pflock_t {
  ten_rwlock_t base;
  struct {
    ten_atomic_t in;
    ten_atomic_t out;
  } rd, wr;
} ten_pflock_t;

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
#define TEN_PFLOCK_WBITS 0x3              /* Writer bits in reader. */
#define TEN_PFLOCK_PRES 0x2               /* Writer present bit. */
#define TEN_PFLOCK_PHID 0x1               /* Phase ID bit. */
#define TEN_PFLOCK_LSB 0xFFFFFFFFFFFFFFF0 /* reader bits. */
#define TEN_PFLOCK_RINC 0x10              /* Reader increment. */

static int __ten_pflock_init(ten_rwlock_t *rwlock) {
  assert(rwlock && ten_rwlock_check_integrity(rwlock));

  ten_pflock_t *pflock = (ten_pflock_t *)rwlock;

  pflock->rd.in = 0;
  pflock->rd.out = 0;
  pflock->wr.in = 0;
  pflock->wr.out = 0;

  return 0;
}

static void __ten_pflock_deinit(ten_rwlock_t *rwlock) {
  assert(rwlock && ten_rwlock_check_integrity(rwlock));

  // do nothing!
}

static int __ten_pflock_lock(ten_rwlock_t *rwlock, int reader) {
  assert(rwlock && ten_rwlock_check_integrity(rwlock));

  ten_pflock_t *pflock = (ten_pflock_t *)rwlock;
  int loops = 0;
  if (reader) {
    uint64_t w;

    /*
     * If no writer is present, then the operation has completed
     * successfully.
     */
    w = ten_atomic_fetch_add(&pflock->rd.in, TEN_PFLOCK_RINC) &
        TEN_PFLOCK_WBITS;
    if (w == 0) {
      return 0;
    }

    /* Wait for current write phase to complete. */
    loops = 0;
    while ((ten_atomic_load(&pflock->rd.in) & TEN_PFLOCK_WBITS) == w) {
      TEN_YIELD(loops);
    }
  } else {
    uint64_t ticket, w;

    /* Acquire ownership of write-phase.
     * This is same as ten_tickelock_lock().
     */
    ticket = ten_atomic_fetch_add(&pflock->wr.in, 1);

    loops = 0;
    while (ten_atomic_load(&pflock->wr.out) != ticket) {
      TEN_YIELD(loops);
    }

    /*
     * Acquire ticket on read-side in order to allow them
     * to flush. Indicates to any incoming reader that a
     * write-phase is pending.
     *
     * The load of rd.out in wait loop could be executed
     * speculatively.
     */
    w = TEN_PFLOCK_PRES | (ticket & TEN_PFLOCK_PHID);
    ticket = ten_atomic_fetch_add(&pflock->rd.in, w);

    /* Wait for any pending readers to flush. */
    loops = 0;
    while (ten_atomic_load(&pflock->rd.out) != ticket) {
      TEN_YIELD(loops);
    }
  }

  return 0;
}

static int __ten_pflock_unlock(ten_rwlock_t *rwlock, int reader) {
  assert(rwlock && ten_rwlock_check_integrity(rwlock));

  ten_pflock_t *pflock = (ten_pflock_t *)rwlock;

  if (reader) {
    ten_atomic_fetch_add(&pflock->rd.out, TEN_PFLOCK_RINC);
  } else {
    /* Migrate from write phase to read phase. */
    ten_atomic_fetch_and(&pflock->rd.in, TEN_PFLOCK_LSB);

    /* Allow other writers to continue. */
    ten_atomic_fetch_add(&pflock->wr.out, 1);
  }

  return 0;
}

static inline ten_rwlock_t *__ten_pflock_create() {
  ten_pflock_t *pflock = (ten_pflock_t *)malloc(sizeof(ten_pflock_t));
  if (pflock == NULL) {
    return NULL;
  }

  memset(pflock, 0, sizeof(ten_pflock_t));
  ten_signature_set(&pflock->base.signature, TEN_RWLOCK_SIGNATURE);
  pflock->base.op.init = __ten_pflock_init;
  pflock->base.op.deinit = __ten_pflock_deinit;
  pflock->base.op.lock = __ten_pflock_lock;
  pflock->base.op.unlock = __ten_pflock_unlock;
  return &pflock->base;
}

typedef struct ten_native_t {
  ten_rwlock_t base;
#if defined(_WIN32)
  SRWLOCK native;
#else
  pthread_rwlock_t native;
#endif
} ten_native_t;

#if defined(_WIN32)

static int __ten_native_init(ten_rwlock_t *rwlock) {
  ten_native_t *native = (ten_native_t *)rwlock;
  InitializeSRWLock(&native->native);
  return 0;
}

static void __ten_native_deinit(ten_rwlock_t *rwlock) {}

static int __ten_native_lock(ten_rwlock_t *rwlock, int reader) {
  ten_native_t *native = (ten_native_t *)rwlock;
  if (reader) {
    AcquireSRWLockShared(&native->native);
  } else {
    AcquireSRWLockExclusive(&native->native);
  }
  return 0;
}

static int __ten_native_unlock(ten_rwlock_t *rwlock, int reader) {
  ten_native_t *native = (ten_native_t *)rwlock;
  if (reader) {
    ReleaseSRWLockShared(&native->native);
  } else {
    ReleaseSRWLockExclusive(&native->native);
  }
  return 0;
}

#else

static int __ten_native_init(ten_rwlock_t *rwlock) {
  assert(rwlock && ten_rwlock_check_integrity(rwlock));

  ten_native_t *native = (ten_native_t *)rwlock;
  return pthread_rwlock_init(&native->native, NULL);
}

static void __ten_native_deinit(ten_rwlock_t *rwlock) {
  assert(rwlock && ten_rwlock_check_integrity(rwlock));

  ten_native_t *native = (ten_native_t *)rwlock;
  pthread_rwlock_destroy(&native->native);
}

static int __ten_native_lock(ten_rwlock_t *rwlock, int reader) {
  assert(rwlock && ten_rwlock_check_integrity(rwlock));

  ten_native_t *native = (ten_native_t *)rwlock;
  return reader ? pthread_rwlock_rdlock(&native->native)
                : pthread_rwlock_wrlock(&native->native);
}

static int __ten_native_unlock(ten_rwlock_t *rwlock, int reader) {
  assert(rwlock && ten_rwlock_check_integrity(rwlock));

  ten_native_t *native = (ten_native_t *)rwlock;
  return pthread_rwlock_unlock(&native->native);
}

#endif

static inline ten_rwlock_t *__ten_native_create() {
  ten_native_t *native = (ten_native_t *)malloc(sizeof(ten_native_t));
  if (native == NULL) {
    return NULL;
  }

  memset(native, 0, sizeof(ten_native_t));
  ten_signature_set(&native->base.signature, TEN_RWLOCK_SIGNATURE);
  native->base.op.init = __ten_native_init;
  native->base.op.deinit = __ten_native_deinit;
  native->base.op.lock = __ten_native_lock;
  native->base.op.unlock = __ten_native_unlock;
  return &native->base;
}

ten_rwlock_t *ten_rwlock_create(TEN_RW_FAIRNESS fair) {
  ten_rwlock_t *rwlock = NULL;
  switch (fair) {
    case TEN_RW_PHASE_FAIR:
      rwlock = __ten_pflock_create();
      break;
    case TEN_RW_NATIVE:
      rwlock = __ten_native_create();
      break;
    default:
      break;
  }

  if (UNLIKELY(!rwlock)) {
    return NULL;
  }

  assert(rwlock && ten_rwlock_check_integrity(rwlock));

  if (__ten_rwlock_base_init(rwlock) != 0) {
    free(rwlock);
    return NULL;
  }

  if (LIKELY(rwlock->op.init != NULL)) {
    if (rwlock->op.init(rwlock) != 0) {
      ten_rwlock_destroy(rwlock);
      return NULL;
    }
  }

  return rwlock;
}

void ten_rwlock_destroy(ten_rwlock_t *lock) {
  if (UNLIKELY(!lock)) {
    return;
  }

  assert(lock && ten_rwlock_check_integrity(lock));

  if (LIKELY(lock->op.deinit != NULL)) {
    lock->op.deinit(lock);
  }

  __ten_rwlock_base_deinit(lock);

  free(lock);
}

int ten_rwlock_lock(ten_rwlock_t *lock, int reader) {
  if (UNLIKELY(!lock)) {
    return -1;
  }

  assert(lock && ten_rwlock_check_integrity(lock));

  if (LIKELY(lock->op.lock != NULL)) {
    return lock->op.lock(lock, reader);
  }

  return -1;
}

int ten_rwlock_unlock(ten_rwlock_t *lock, int reader) {
  if (UNLIKELY(!lock)) {
    return -1;
  }

  assert(lock && ten_rwlock_check_integrity(lock));

  if (LIKELY(lock->op.unlock != NULL)) {
    return lock->op.unlock(lock, reader);
  }

  return -1;
}
