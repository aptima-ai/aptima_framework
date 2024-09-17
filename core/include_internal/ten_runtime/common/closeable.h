//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/container/list.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/sanitizer/thread_check.h"

#define TEN_CLOSEABLE_SIGNATURE 0x7263656c6f736564U

typedef enum TEN_CLOSEABLE_STATE {
  TEN_CLOSEABLE_STATE_ALIVE,
  TEN_CLOSEABLE_STATE_CLOSING,
  TEN_CLOSEABLE_STATE_CLOSED,
} TEN_CLOSEABLE_STATE;

/**
 * @brief The enum is used to represent whether the 'on_closed_done()' callback
 * of ten_closeable_t will be invoked in the thread where ten_closeable_t
 * resides.
 *
 * Suppose that there are a 'ten_closeable_t' instance (we named it 'CA'), and
 * another instance (we named it 'CB') are interested in the 'closed' event of
 * CA. After CA is closed, CB will receive the 'on_closed' event of CA, and then
 * CA should receive the 'on_closed_done' acked from CB. As CA and CB might
 * belongs to different threads, the 'on_closed_done' event maybe delivered to
 * CA from other threads, and normally, we prefer to use the runloop tasks to
 * ensure the thread safety in this case. However, the runloop of CA might be
 * already closed before the 'on_closed_done' event being delivered to CA.
 *
 * The logic of closing is determined by the implementation of the runloop, we
 * cannot make any restrictions. In other words, the runloop might or might not
 * be able to deliver events after it is closed.
 *
 * Usually, the existence of the runloop of CA is in the following two forms.
 *
 * - CA owns a runloop.
 *
 *   In this case, the runloop will be closed before the CA is closed, as the
 *   runloop is a resource owned by CA.
 *
 * - CA does not have its own runloop. In this case, CA uses the runloop belongs
 *   to another 'ten_closeable_t' instance (we named it 'CC'). In the semantics
 *   of ten_closeable_t, CA depends on CC.
 *
 *   In this case, CC might start to close itself after it receives the
 *   'on_closed' event of CA. So the runloop could not be used to deliver the
 *   'on_closed_done' event as it might be already closed.
 *
 * Thus, the 'on_closed_done' could be received in the following two conditions.
 *
 * - In the thread of CA.
 *
 *   * Because CA and CB (who is interested in the 'closed' event of CA) are in
 *     the same thread.
 *
 *   * Or because the runloop of CA (including the runloop CA depends on) is
 *     still able to deliver events when it is closed, so CB could deliver the
 *     'on_closed_done()' ack to CA through the runloop of CA.
 *
 * - Out of the thread of CA.
 *
 *   * CA and CB are in the different threads, and the runloop of CA could not
 *     be able to deliver events when it is closed. Therefore, the
 *     'on_closed_done()' of CA would be called in the thread of CB.
 *
 * In the meanwhile, I can not use the runloop of my owner to deliver the
 * 'on_closed_done' event of mine, because not all the 'ten_closeable_t'
 * instances always have an owner.
 */
typedef enum TEN_CLOSEABLE_ON_CLOSED_DONE_MODE {
  // The 'on_closed_done()' of mine is called in my own thread, it means that
  // the accesses to me is thread safe.
  TEN_CLOSEABLE_ON_CLOSED_DONE_MODE_IN_OWN_THREAD,

  // The 'on_closed_done()' of mine is called in other threads. One possible use
  // case of this mode is that others can not use the runloop I am currently in
  // to the 'on_closed_done' event to me as the runloop can not be used when I
  // am closed. In this case, we have to ensure the thread safety in help of
  // some mutex locks.
  TEN_CLOSEABLE_ON_CLOSED_DONE_MODE_OUT_OWN_THREAD,
} TEN_CLOSEABLE_ON_CLOSED_DONE_MODE;

typedef struct ten_closeable_t ten_closeable_t;

/**
 * @brief This function is used to indicate that the closed event of @a self has
 * been processed by @a who_have_interest_on_me.
 */
typedef void (*ten_closeable_on_closed_done_func_t)(
    ten_closeable_t *self, void *who_have_interest_on_me, void *on_closed_data);

/**
 * @brief This function is used to notify others (i.e., @a
 * who_have_interest_on_me) who are interested in the 'closed' event of mine.
 *
 * @param self The instance who implements the 'ten_closeable_t' interface.
 * @param who_have_interest_on_me The instance who is interested in the 'closed'
 * event of mine.
 * @param on_closed_data The data bound to the 'on_closed' callback.
 * @param on_closed_done The callback to notify the 'closed' event is handled by
 * @a other completely. This callback must not be NULL, and @a other must call
 * this callback when it handles the 'closed' event completely.
 */
typedef void (*ten_closeable_on_closed_func_t)(
    ten_closeable_t *self, void *who_have_interest_on_me, void *on_closed_data,
    ten_closeable_on_closed_done_func_t on_closed_done);

/**
 * @brief This function is used to notify others (i.e., @a
 * who_have_interest_on_me) who are interested in the 'on_closed_all_done' event
 * of mine.
 *
 * @param self The instance who implements the 'ten_closeable_t' interface.
 * @param who_have_interest_on_me The instance who is interested in the
 * 'on_closed_all_done' event of mine.
 * @param on_closed_all_done_data The data bound to the 'on_closed_all_done'
 * callback.
 *
 * @note @a self might be destroyed by its owner after its owner receives the
 * 'on_closed_all_done' event, so it's not safe for others to access the memory
 * of @a self in this callback.
 */
typedef void (*ten_closeable_on_closed_all_done_func_t)(
    ten_closeable_t *self, void *who_have_interest_on_me,
    void *on_closed_all_done_data);

/**
 * @brief This function is used to notify others (@a who_have_interest_on_me)
 * who are interested in my intend_to_close event.
 */
typedef void (*ten_closeable_on_intend_to_close_func_t)(
    ten_closeable_t *self, void *who_have_interest_on_me,
    void *on_intend_to_close_data);

typedef struct ten_closeable_on_intend_to_close_item_t {
  void *who_have_interest_on_me;
  ten_closeable_on_intend_to_close_func_t on_intend_to_close_cb;
  void *on_intend_to_close_data;
} ten_closeable_on_intend_to_close_item_t;

typedef struct ten_closeable_on_closed_item_t {
  void *who_have_interest_on_me;
  ten_closeable_on_closed_func_t on_closed_cb;
  void *on_closed_data;
} ten_closeable_on_closed_item_t;

typedef struct ten_closeable_on_closed_all_done_item_t {
  void *who_have_interest_on_me;
  ten_closeable_on_closed_all_done_func_t on_closed_all_done_cb;
  void *on_closed_all_done_data;
} ten_closeable_on_closed_all_done_item_t;

/**
 * @brief This structure is used to collect information about which other
 * instances, as well as their interest in which closing events of a
 * ten_closeable_t instance.
 *
 * @note Someone is not always interested in all closing events of mine, so we
 * prefer to use three different queues here.
 */
typedef struct ten_closeable_be_notified_resources_t {
  // Is interested in 'intend_to_close' event. The type of the element is
  // 'ten_closeable_on_intend_to_close_item_t'.
  ten_list_t on_intend_to_close_queue;

  // @{
  // Is interested in 'closed' event. The type of the element is
  // 'ten_closeable_on_closed_item_t'.
  ten_list_t on_closed_queue;

  TEN_CLOSEABLE_ON_CLOSED_DONE_MODE on_closed_done_mode;
  ten_closeable_on_closed_done_func_t on_closed_done_cb;

  // The expected count of 'on_closed_done' event I will receive from others, if
  // the 'on_closed_done_mode' is 'OUT_OWN_THREAD'. As the 'on_closed_done'
  // event is called in other threads, it's unsafe to access the fields such
  // as 'on_closed_queue' and 'on_closed_all_done_queue' of this structure. So
  // we have to use the mutex lock to ensure the safety.
  ten_mutex_t *on_closed_done_mutex;

  // The expected count of 'on_closed_done' event I will receive from others,
  // it's the size of the 'on_closed_queue'. It's just a faster way to determine
  // whether I have received all 'on_closed_done' events from others, rather
  // than locking all the 'on_closed_queue'.
  size_t expected_on_closed_done_count;
  // @}

  // Is interested in 'closed_all_done' event. The type of the element is
  // 'ten_closeable_on_closed_all_done_item_t'.
  ten_list_t on_closed_all_done_queue;
} ten_closeable_be_notified_resources_t;

/**
 * @brief This function is used to determine whether @a self is the higher-level
 * root when @a underlying_resource wants to close.
 */
typedef bool (*ten_closeable_is_closing_root_func_t)(
    ten_closeable_t *self, ten_closeable_t *underlying_resource,
    void *is_closing_root_data);

/**
 * @brief This structure is used to represent which ten_closeable_t instance
 * (i.e., @a belong_to) owns a given ten_closeable_t instance.
 */
typedef struct ten_closeable_belong_to_info_t {
  ten_closeable_t *belong_to;

  ten_closeable_is_closing_root_func_t is_closing_root_cb;
  void *is_closing_root_data;
} ten_closeable_belong_to_info_t;

/**
 * @brief This function is used to indicate that the action of closing the
 * ten_closeable_t itself has been completed entirely.
 */
typedef void (*ten_closeable_action_to_close_myself_done_func_t)(
    ten_closeable_t *self, void *action_to_close_myself_data);

/**
 * @brief This function is used to represent a customized action for a
 * 'ten_closeable_t' to close itself.
 */
typedef void (*ten_closeable_action_to_close_myself_func_t)(
    ten_closeable_t *self, void *action_to_close_myself_data,
    ten_closeable_action_to_close_myself_done_func_t
        action_to_close_myself_done);

typedef struct ten_closeable_action_to_close_myself_t {
  ten_closeable_action_to_close_myself_func_t action_to_close_myself_cb;
  void *action_to_close_myself_data;
} ten_closeable_action_to_close_myself_t;

/**
 * @brief The standard interface for closing a resource.
 *
 * Basically, if A wants to be closed, there are some stages during the closing
 * flow.
 *
 * 1) Stage: Make a 'intend_to_close' announcement.
 *    In this stage, a 'ten_closeable_t' tries to announce the intend_to_close
 *    event to all others who are interested in this event.
 *
 * 2) Stage: Determine the closing root.
 *    In this stage, all possible closing roots will be determined.
 *
 * 3) Stage: Start to close.
 *    In this stage, the closing flow will be started from all roots.
 *
 * 4) Stage: Close owned 'ten_closeable_t' resources.
 *    In this stage, all owned 'ten_closeable_t' resources of a
 *    'ten_closeable_t' are closed.
 *
 * 5) Stage: Close owned non-'ten_closeable_t' resources.
 *    In this stage, all owned non-'ten_closeable_t' resources of a
 *    'ten_closeable_t' are closed.
 *
 * 6) Stage: On closed, make a 'closed' announcement.
 *    When a 'ten_closeable_t' is closed completed, it will notify others, who
 *    are interested in my 'closed' event, that I am closed.
 *
 * 7) Stage: All subscribers interested in the 'on_closed' event have completed
 *    handling the 'on_closed' event, then make a 'on_closed_all_done'
 *    announcement. In other words, when a 'ten_closeable_t' receives all the
 *    'on_closed_done' callbacks, it will notify others who are interested in my
 *    'on_closed_all_done' event. Please note that this 'ten_closeable_t' might
 *    be destroyed in this stage by its owner.
 */
typedef struct ten_closeable_t {
  ten_signature_t signature;

  // All operations _must_ be called in the same thread.
  ten_sanitizer_thread_check_t thread_check;

  TEN_CLOSEABLE_STATE state;

  // The offset of this 'ten_closeable_t' object in the implementation who
  // implements the 'ten_closeable_t' interface. It will be used to get the raw
  // pointer of the implementation.
  //
  // We want to use the following coding style to ensure that the
  // 'implementation' is paired one-to-one with 'ten_closeable_t'.
  //
  // - The 'ten_closeable_t' is an embedded member of 'implementation' rather
  //   than a pointer.
  //
  // Ex:
  //   struct some_impl_t {
  //     // other fields.
  //
  //     ten_closeable_t closeable;
  //
  //     // other fields.
  //   }
  //
  // The above example is similar to the following codes in other programming
  // language:
  //
  //   class some_impl_t implements ten_closeable_t;
  //
  // We want to represent the following two key points:
  //
  // 1. The implementation is a kind of 'ten_closeable_t'.
  //
  // 2. One 'ten_closeable_t' could only be paired with one implementation. The
  //    implementation is closing when its embedded 'ten_closeable_t' is
  //    closing; the implementation is closed when its embedded
  //    'ten_closeable_t' is closed.
  //
  // So we keep the offset rather than the raw pointer here.
  ptrdiff_t offset_in_impl;

  /**
   * @brief I want to customize the logic that determines whether I am the
   * closing root, instead of using the default behavior of recursively
   * searching through belong_to_resources.
   *
   * @note The second parameter of the callback (i.e., @a underlying_resource)
   * will be NULL in this case.
   */
  ten_closeable_is_closing_root_func_t is_closing_root_myself_cb;
  void *is_closing_root_myself_data;

  ten_list_t belong_to_resources;  // ten_closeable_belong_to_info_t

  // This list is used to store those who are interested in my 'closed' events.
  ten_closeable_be_notified_resources_t be_notified_resources;

  ten_list_t underlying_resources;  // ten_closeable_t

  // The resources depend on me.
  //
  // If A depends on B, it means B could only be closed after A is closed. So A
  // will be kept in the 'be_depended_on_resources' list of B, then B could know
  // whether all resources depend on it are closed.
  ten_list_t be_depended_on_resources;  // ten_closeable_t

  ten_closeable_action_to_close_myself_t action_to_close_myself;
} ten_closeable_t;

TEN_RUNTIME_API bool ten_closeable_check_integrity(ten_closeable_t *self,
                                                 bool thread_check);

/**
 * @example An implementation whose name is 'some_impl_t' and the name of the
 * 'ten_closeable_t' field in it is 'closeable'. The calling of this function in
 * the implementation will be as follows:
 *
 * ten_closeable_init(&impl->closeable,
 *   offsetof(some_impl_t, closeable)))
 */
TEN_RUNTIME_API void ten_closeable_init(ten_closeable_t *self, ptrdiff_t offset);

TEN_RUNTIME_API void ten_closeable_deinit(ten_closeable_t *self);

TEN_RUNTIME_API void ten_closeable_close(ten_closeable_t *self);

TEN_RUNTIME_API bool ten_closeable_is_closed(ten_closeable_t *self);

TEN_RUNTIME_API bool ten_closeable_is_closing(ten_closeable_t *self);

/**
 * @brief Enable @a self to own a resource (@a underlying_resource).
 *
 * @param on_closed_all_done_cb Required. The owner (i.e., @a self) _must_
 * receive the 'on_closed_all_done' event of the underlying resource (i.e.,
 * @a underlying_resource) to destroy the underlying resource.
 */
TEN_RUNTIME_API void ten_closeable_add_underlying_resource(
    ten_closeable_t *self, ten_closeable_t *underlying_resource,
    ten_closeable_is_closing_root_func_t is_closing_root_cb,
    void *is_closing_root_data,
    ten_closeable_on_intend_to_close_func_t on_intend_to_close_cb,
    void *on_intend_to_close_data,
    ten_closeable_on_closed_all_done_func_t on_closed_all_done_cb,
    void *on_closed_all_done_data);

/**
 * @brief @a self depends on @a depend_resource, @a self will be an item of
 * 'be_depended_on_resources' of @a depend_resource.
 *
 * @param on_closed_cb This is a function of @a depend_resource, which will be
 * registered in @a self. This callback is used to notify @a depend_resource
 * that someone depends on it has been closed.
 *
 * @param on_intend_to_close_cb This is a function of @a self, which will be
 * registered in @a depend_resource. This callback is used to notify @a self
 * when its dependency intends to close.
 */
TEN_RUNTIME_API void ten_closeable_add_depend_resource(
    ten_closeable_t *self, ten_closeable_t *depend_resource,
    ten_closeable_on_closed_func_t on_closed_cb, void *on_closed_data,
    ten_closeable_on_intend_to_close_func_t on_intend_to_close_cb,
    void *on_intend_to_close_data);

/**
 * @brief @a self depends on @a depend_resource. Use this function to release
 * the dependency relationship from @a depend_resource only if needed, ex: @a
 * self is closed.
 */
TEN_RUNTIME_API void ten_closeable_remove_depend_resource(
    ten_closeable_t *self, ten_closeable_t *depend_resource);

/**
 * @brief @a self depends on @a depend_resource. Use this function to release
 * the dependency relationship between @a self and @a depend_resource if needed.
 */
TEN_RUNTIME_API void ten_closeable_remove_depend_resource_bidirectional(
    ten_closeable_t *self, ten_closeable_t *depend_resource);

/**
 * @note Add @a who_have_interest_on_me who is interested in my various closing
 * event to the closeable management of mine (i.e., @a self).
 *
 * @param self The instance who implements the 'ten_closeable_t' interface.
 * @param who_have_interest_on_me The instance who is interested in various
 * closing event of @a self.
 */
TEN_RUNTIME_API void ten_closeable_add_be_notified(
    ten_closeable_t *self, void *who_have_interest_on_me,
    // intend_to_close event.
    ten_closeable_on_intend_to_close_func_t on_intend_to_close_cb,
    void *on_intend_to_close_data,
    // closed event.
    ten_closeable_on_closed_func_t on_closed_cb, void *on_closed_data,
    // closed_all_done event.
    ten_closeable_on_closed_all_done_func_t on_closed_all_done_cb,
    void *on_closed_all_done_data);

/**
 * @brief @a who_have_interest_on_me does not interest in the various closing
 * events of @a self any more. Ex: @a self is closed.
 */
TEN_RUNTIME_API void ten_closeable_remove_be_notified(
    ten_closeable_t *self, void *who_have_interest_on_me);

TEN_RUNTIME_API void ten_closeable_intend_to_close(ten_closeable_t *self,
                                                 void *intend_to_close_data);

TEN_RUNTIME_API void ten_closeable_action_to_close_myself_done(
    ten_closeable_t *self, void *on_close_myself_data);

TEN_RUNTIME_API void ten_closeable_set_action_to_close_myself(
    ten_closeable_t *self,
    ten_closeable_action_to_close_myself_func_t action_to_close_myself_cb,
    void *action_to_close_myself_data);

TEN_RUNTIME_API void ten_closeable_set_is_closing_root_myself(
    ten_closeable_t *self,
    ten_closeable_is_closing_root_func_t is_closing_root_myself_cb,
    void *is_closing_root_myself_data);
