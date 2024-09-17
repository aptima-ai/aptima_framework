//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/common/closeable.h"
#include "include_internal/ten_runtime/protocol/asynced/internal.h"
#include "include_internal/ten_runtime/protocol/asynced/protocol_asynced.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/lib/ref.h"
#include "ten_utils/macro/mark.h"

// @{
// intend_to_close.

// As 'ten_protocol_asynced_impl_on_intend_to_close()' and
// 'ten_protocol_asynced_handle_intends_to_close()' will call each other, we
// declare it here.
static void ten_protocol_asynced_impl_on_intend_to_close(
    ten_closeable_t *impl, void *self_, void *on_intend_to_close_data);

static void ten_protocol_asynced_handle_intends_to_close(
    ten_protocol_asynced_t *self, TEN_UNUSED void *arg) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(self->impl_closeable &&
                 ten_closeable_check_integrity(self->impl_closeable, true),
             "Access across threads.");

  ten_protocol_asynced_impl_on_intend_to_close(self->impl_closeable, self,
                                               NULL);
}

// When the implementation protocol wants to close, it needs to notify the
// protocol in the TEN world, and this notification action would across threads.
static void ten_protocol_asynced_impl_on_intend_to_close(
    ten_closeable_t *impl, void *self_,
    TEN_UNUSED void *on_intend_to_close_data) {
  TEN_ASSERT(
      impl && ten_closeable_check_integrity(impl, true),
      "This function is always called in the implementation protocol thread.");

  ten_protocol_asynced_t *self = (ten_protocol_asynced_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");

  ten_protocol_asynced_post_task_to_ten(
      self, ten_protocol_asynced_handle_intends_to_close,
      ten_protocol_asynced_intends_to_close_task, NULL);
}
// @}

static void ten_protocol_asynced_impl_on_closed(
    ten_closeable_t *impl, void *self_, void *on_closed_data,
    ten_closeable_on_closed_done_func_t on_closed_done) {
  TEN_ASSERT(
      impl && ten_closeable_check_integrity(impl, true),
      "This function is always called in the implementation protocol thread.");

  // The ten_protocol_asynced_t::closeable is not the directly owner of
  // ten_protocol_asynced_t::impl_closeable, as they are in different threads.
  // Since the async protocol only has the external implementation protocol as
  // its resource, there is not much else to do within the on_closed() callback.
  // It simply needs to invoke the on_closed_done() callback to notify @a impl
  // that all the tasks it needs to do for the 'closed' event have been
  // completed.
  if (on_closed_done) {
    on_closed_done(impl, self_, on_closed_data);
  }
}

static void ten_protocol_asynced_impl_on_closed_all_done(
    TEN_UNUSED ten_closeable_t *impl, void *self_,
    TEN_UNUSED void *on_closed_all_done_data) {
  ten_protocol_asynced_t *self = (ten_protocol_asynced_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");

  // The implementation protocol has been closed, so the connection could not be
  // in migration.
  ten_protocol_asynced_post_task_to_ten(
      self, NULL, ten_protocol_asynced_on_impl_closed_task, NULL);
}

void ten_protocol_asynced_set_impl_closeable(ten_protocol_asynced_t *self,
                                             ten_closeable_t *impl) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(
      impl && ten_closeable_check_integrity(impl, true),
      "This function is always called in the implementation protocol thread.");

  self->impl_closeable = impl;

  // The closeable of the implementation (i.e., 'self->impl_closeable') belongs
  // to the implementation protocol thread, so we have to customize the
  // 'intend_to_close' and 'on_closed' hook to do the thread context switch.
  ten_closeable_add_be_notified(
      self->impl_closeable, self, ten_protocol_asynced_impl_on_intend_to_close,
      NULL, ten_protocol_asynced_impl_on_closed, NULL,
      ten_protocol_asynced_impl_on_closed_all_done, NULL);
}

static bool ten_protocol_asynced_is_closing_root_myself(
    TEN_UNUSED ten_closeable_t *self, TEN_UNUSED ten_closeable_t *underlying,
    TEN_UNUSED void *on_closing_root_not_found_data) {
  // The closeable of the implementation protocol will be the root in its own
  // world, as it could _not_ be the directly underlying resource of the
  // ten_protocol_asynced_t::closeable. In other words, the
  // 'belong_to_resources' of the closeable of the implementation protocol is
  // EMPTY. However, the closeable of the implementation protocol could not be
  // the closing root, as the resources in the implementation world are a
  // subtree of the base protocol (i.e., ten_protocol_t) in the ten world. The
  // duty of ten_protocol_asynced_t::closeable is to connect the resources in
  // the two worlds.
  return false;
}

void ten_protocol_asynced_set_default_closeable_behavior(
    ten_closeable_t *impl) {
  TEN_ASSERT(impl && ten_closeable_check_integrity(impl, true),
             "Access across threads.");

  ten_closeable_set_is_closing_root_myself(
      impl, ten_protocol_asynced_is_closing_root_myself, NULL);
}

void ten_protocol_asynced_on_impl_closed_async(ten_protocol_asynced_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  ten_protocol_t *base_protocol = &self->base;
  TEN_ASSERT(base_protocol, "Should not happen.");
  TEN_ASSERT(
      // TEN_NOLINTNEXTLINE(thread-check)
      // By design, it can be called from any threads when the
      // implementation is closed. When the implementation is
      // closing, it may have to switch to its own thread to do some
      // cleanup. When the implementation closure is done, it can
      // call this function directly from its thread without needing
      // to switch to the thread of the base protocol.
      ten_protocol_check_integrity(base_protocol, false), "Should not happen.");

  ten_protocol_asynced_post_task_to_ten(
      self, NULL, ten_protocol_asynced_on_impl_closed_task, NULL);
}

void ten_protocol_asynced_close_impl(ten_protocol_asynced_t *self,
                                     TEN_UNUSED void *arg) {
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(
      self->impl_closeable &&
          ten_closeable_check_integrity(self->impl_closeable, true),
      "This function is always called in the implementation protocol thread.");

  ten_closeable_close(self->impl_closeable);

  ten_ref_dec_ref(&self->base.ref);
}
