//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
/**
 * @file
 * @brief A runloop is the core of an asynchronous event-driven programming
 * model, which handles all the events, timers, signals and message queues in an
 * application.
 */
#pragma once

#include "axis_utils/axis_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define axis_RUNLOOP_EVENT2 "event2"
#define axis_RUNLOOP_UV "uv"
#define axis_RUNLOOP_BARE "bare"

/**
 * @brief This data structure represents a runloop.
 */
typedef struct axis_runloop_t axis_runloop_t;

/**
 * @brief This data structure represents an asynchronous event signal.
 */
typedef struct axis_runloop_async_t axis_runloop_async_t;

/**
 * @brief This data structure represents a timer.
 */
typedef struct axis_runloop_timer_t axis_runloop_timer_t;

typedef struct axis_thread_t axis_thread_t;

axis_UTILS_API bool axis_runloop_check_integrity(axis_runloop_t *self,
                                               bool check_thread);

/**
 * @brief Create a runloop.
 * @param type The implementation of the runloop.
 *             Create from a default one if |type| == NULL
 * @return The runloop. NULL if failed.
 */
axis_UTILS_API axis_runloop_t *axis_runloop_create(const char *type);

/**
 * @brief Attach to an existing runloop with "raw loop pointer"
 *
 * @param type The implementation of the runloop. Must _not_ be "NULL".
 * @param raw The "raw loop pointer", for example ev_base* if type == "event2"
 * @return The runloop.
 * @note Be careful that |raw| must has current |type| otherwise crash happens.
 */
axis_UTILS_API axis_runloop_t *axis_runloop_attach(const char *type, void *raw);

/**
 * @return true if @a loop is attached to another raw runloop.
 */
axis_UTILS_API bool axis_runloop_is_attached(axis_runloop_t *loop);

/**
 * @brief Destroy a runloop.
 * @param loop The runloop to destroy.
 * @note Be _very_ careful that if loop is from |axis_runloop_attach|, timers and
 *       events created from |loop| may still be triggered unless you close them
 *       all before detaching.
 */
axis_UTILS_API void axis_runloop_destroy(axis_runloop_t *loop);

/**
 * @brief Get current loop.
 * @return The current loop. NULL if no loop running.
 */
axis_UTILS_API axis_runloop_t *axis_runloop_current(void);

/**
 * @brief Get underlying loop object.
 * @param loop The runloop.
 * @return The underlying loop object. NULL if failed.
 *         For example, libevent loop will return event_base*
 */
axis_UTILS_API void *axis_runloop_get_raw(axis_runloop_t *loop);

/**
 * @brief Run the loop.
 * @param loop The runloop.
 * @note This function will block until the loop is stopped.
 *       If loop is coming from |axis_runloop_attach|, nothing would happen.
 */
axis_UTILS_API void axis_runloop_run(axis_runloop_t *loop);

/**
 * @brief Stop the loop.
 * @param loop The runloop.
 * @note Stop a loop from |axis_runloop_create| will stop task queue as well as
 *       the underlying io loop.
 *       Stop a loop from |axis_runloop_attach| will only stop task queue.
 */
axis_UTILS_API void axis_runloop_stop(axis_runloop_t *loop);

typedef void (*axis_runloop_on_stopped_func_t)(axis_runloop_t *loop, void *data);

/**
 * @brief Register a callback function which will be called when @a loop is
 * stopped completely.
 */
axis_UTILS_API void axis_runloop_set_on_stopped(
    axis_runloop_t *loop, axis_runloop_on_stopped_func_t on_stopped,
    void *on_stopped_data);

/**
 * @brief Close the loop.
 * @param loop The runloop.
 * @note Notify a loop to close its relevant resources before stopping.
 */
axis_UTILS_API void axis_runloop_close(axis_runloop_t *loop);

/**
 * @brief Check whether the loop is running.
 * @param loop The runloop.
 * @return 1 if the loop is running, 0 otherwise.
 */
axis_UTILS_API int axis_runloop_alive(axis_runloop_t *loop);

/**
 * @brief Create an async signal
 * @param type The implementation of signal.
 *             Create from a default one if |type| == NULL
 * @return The signal. NULL if failed.
 */
axis_UTILS_API axis_runloop_async_t *axis_runloop_async_create(const char *type);

/**
 * @brief Close an async signal.
 * @param async The signal.
 * @param close_cb The callback to be called when the signal is closed.
 */
axis_UTILS_API void axis_runloop_async_close(
    axis_runloop_async_t *async, void (*close_cb)(axis_runloop_async_t *));

/**
 * @brief Destroy an async signal.
 * @param async The signal.
 */
axis_UTILS_API void axis_runloop_async_destroy(axis_runloop_async_t *async);

/**
 * @brief Notify the signal.
 * @param async The signal.
 * @return 0 if success, -1 otherwise.
 * @note The signal callback will be called in the thread of |loop| that initted
 */
axis_UTILS_API int axis_runloop_async_notify(axis_runloop_async_t *async);

/**
 * @brief Bind an async signal to a runloop.
 * @param async The signal.
 * @param loop The runloop.
 * @param callback The callback to be called when the signal is notified.
 * @return 0 if success, -1 otherwise.
 * @note The implementation of |loop| should be the same as the implementation
 *       of |async|, otherwise the behavior is undefined.
 */
axis_UTILS_API int axis_runloop_async_init(
    axis_runloop_async_t *async, axis_runloop_t *loop,
    void (*callback)(axis_runloop_async_t *));

typedef void (*axis_runloop_task_func_t)(void *from, void *arg);

/**
 * @brief Create an async task and insert it in front.
 * @param loop The runloop.
 * @param task_cb The callback to be called when the task is executed.
 * @param from The pointer of poster.
 * @param arg The argument to be passed to the callback.
 * @return 0 if success, -1 otherwise.
 */
axis_UTILS_API int axis_runloop_post_task_front(axis_runloop_t *loop,
                                              axis_runloop_task_func_t task_cb,
                                              void *from, void *arg);

/**
 * @brief Create an async task and insert it in back.
 * @param loop The runloop.
 * @param task_cb The callback to be called when the task is executed.
 * @param from The pointer of poster.
 * @param arg The argument to be passed to the callback.
 * @return 0 if success, -1 otherwise.
 */
axis_UTILS_API int axis_runloop_post_task_tail(axis_runloop_t *loop,
                                             axis_runloop_task_func_t task_cb,
                                             void *from, void *arg);

/**
 * @brief Get pending task size
 * @param loop The runloop.
 * @return The pending task size.
 */
axis_UTILS_API size_t axis_runloop_task_queue_size(axis_runloop_t *loop);

/**
 * @brief Execute all the remaining tasks in the runloop task queue.
 * @param loop The runloop.
 */
axis_UTILS_API void axis_runloop_flush_task(axis_runloop_t *loop);

/**
 * @brief Create a timer in of a runloop
 * @param type The implementation of timer.
 *             Create from a default one if |type| == NULL
 * @param timeout Timeout time in ms.
 * @param periodic Whether the timer is notified periodicity
 * @return The timer. NULL if failed.
 */
axis_UTILS_API axis_runloop_timer_t *axis_runloop_timer_create(const char *type,
                                                            uint64_t timeout,
                                                            uint64_t periodic);

/**
 * @brief Set timeout and callback style
 * @param timer The timer.
 * @param timeout Timeout time in ms.
 * @param periodic Whether the timer is notified periodicity.
 * @return 0 if success, -1 if the timer not valid.
 * @note Will not take effect immediately if already started.
 */
axis_UTILS_API int axis_runloop_timer_set_timeout(axis_runloop_timer_t *timer,
                                                uint64_t timeout,
                                                uint64_t periodic);

/**
 * @brief Bind an timer to a runloop and start.
 * @param timer The timer.
 * @param loop The runloop.
 * @param callback The callback to be called when timer is notified.
 * @return 0 if success, -1 otherwise.
 * @note The implementation of |loop| should be the same as the implementation
 *       of |timer|, otherwise the behavior is undefined.
 */
axis_UTILS_API int axis_runloop_timer_start(
    axis_runloop_timer_t *timer, axis_runloop_t *loop,
    void (*callback)(axis_runloop_timer_t *, void *), void *arg);

/**
 * @brief Stop an timer.
 * @param timer The timer.
 * @param stop_cb The callback to be called when the timer is stopped.
 */
axis_UTILS_API void axis_runloop_timer_stop(axis_runloop_timer_t *timer,
                                          void (*stop_cb)(axis_runloop_timer_t *,
                                                          void *),
                                          void *arg);

/**
 * @brief Stop an timer.
 * @param timer The timer.
 * @param close_cb The callback to be called when the timer is stopped.
 */
axis_UTILS_API void axis_runloop_timer_close(
    axis_runloop_timer_t *timer, void (*close_cb)(axis_runloop_timer_t *, void *),
    void *arg);

/**
 * @brief Destroy an timer.
 * @param timer The timer.
 */
axis_UTILS_API void axis_runloop_timer_destroy(axis_runloop_timer_t *timer);
