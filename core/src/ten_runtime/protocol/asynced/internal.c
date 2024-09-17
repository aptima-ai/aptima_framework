//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/protocol/asynced/internal.h"

#include "include_internal/ten_runtime/common/closeable.h"
#include "include_internal/ten_runtime/protocol/asynced/protocol_asynced.h"
#include "include_internal/ten_runtime/protocol/close.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/lib/ref.h"
#include "ten_utils/macro/field.h"
#include "ten_utils/macro/mark.h"

void ten_protocol_asynced_on_impl_closed_task(void *self_,
                                              TEN_UNUSED void *arg) {
  ten_protocol_asynced_t *self = (ten_protocol_asynced_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_closeable_check_integrity(&self->closeable, true),
             "This function must be called in the ten world.");

  ten_protocol_t *base_protocol = &self->base;
  TEN_ASSERT(ten_protocol_check_integrity(base_protocol, true),
             "Invalid argument.");

  ten_closeable_action_to_close_myself_done(&self->closeable, NULL);

  ten_ref_dec_ref(&base_protocol->ref);
}

static void ten_protocol_asynced_action_to_close_myself(
    ten_closeable_t *closeable, TEN_UNUSED void *action_to_close_myself_data,
    TEN_UNUSED ten_closeable_action_to_close_myself_done_func_t
        action_to_close_myself_done) {
  TEN_ASSERT(closeable && ten_closeable_check_integrity(closeable, true),
             "Access across threads.");

  ten_protocol_asynced_t *self =
      CONTAINER_OF_FROM_OFFSET(closeable, closeable->offset_in_impl);
  TEN_ASSERT(self, "Invalid argument.");

  ten_protocol_t *base_protocol = &self->base;
  TEN_ASSERT(ten_protocol_check_integrity(base_protocol, true),
             "Access across threads.");

  ten_ref_inc_ref(&base_protocol->ref);

  // Note that we can not read 'ten_protocol_asynced_t::impl_closeable' here
  // as it should be accessed in the implementation protocol thread.
  self->post_task_to_impl(self, ten_protocol_asynced_close_impl, NULL);
}

void ten_protocol_asynced_init_closeable(ten_protocol_asynced_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_closeable_init(&self->closeable,
                     offsetof(ten_protocol_asynced_t, closeable));

  // The closure of the ten_protocol_asynced_t is triggered by its owner (i.e.,
  // ten_protocol_t) from the ten world, but the implementation protocol runs in
  // another thread. So what the ten_protocol_asynced_t does in closing itself
  // is to do the thread context switch and to close the implementation in the
  // external thread.
  ten_closeable_set_action_to_close_myself(
      &self->closeable, ten_protocol_asynced_action_to_close_myself, NULL);

  ten_closeable_add_underlying_resource(
      &self->base.closeable, &self->closeable, NULL, NULL,
      (ten_closeable_on_intend_to_close_func_t)
          ten_protocol_on_impl_intends_to_close,
      NULL, ten_protocol_on_impl_closed_all_done, NULL);

  // This field _MUST_ be assigned in the implementation protocol thread.
  self->impl_closeable = NULL;
}

void ten_protocol_asynced_intends_to_close_task(void *self_,
                                                TEN_UNUSED void *arg) {
  ten_protocol_asynced_t *self = (ten_protocol_asynced_t *)self_;
  TEN_ASSERT(self, "Invalid argument.");
  TEN_ASSERT(ten_closeable_check_integrity(&self->closeable, true),
             "This function is always called in the ten world.");

  ten_protocol_t *base_protocol = &self->base;
  TEN_ASSERT(ten_protocol_check_integrity(base_protocol, true),
             "Access across threads.");

  ten_closeable_intend_to_close(&self->closeable, NULL);

  ten_ref_dec_ref(&base_protocol->ref);
}
