//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/common/closeable.h"

#include "ten_utils/macro/check.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_node.h"
#include "ten_utils/container/list_node_ptr.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/lib/mutex.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/macro/memory.h"
#include "ten_utils/sanitizer/thread_check.h"

bool ten_closeable_check_integrity(ten_closeable_t *self, bool thread_check) {
  TEN_ASSERT(self, "Invalid argument.");

  if (ten_signature_get(&self->signature) != TEN_CLOSEABLE_SIGNATURE) {
    return false;
  }

  if (thread_check) {
    return ten_sanitizer_thread_check_do_check(&self->thread_check);
  }

  return true;
}

static void ten_closeable_action_to_close_myself_init(
    ten_closeable_action_to_close_myself_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  self->action_to_close_myself_cb = NULL;
  self->action_to_close_myself_data = NULL;
}

static void ten_closeable_be_notified_resources_init(
    ten_closeable_be_notified_resources_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  self->on_closed_done_mode = TEN_CLOSEABLE_ON_CLOSED_DONE_MODE_IN_OWN_THREAD;
  self->on_closed_done_cb = NULL;

  self->expected_on_closed_done_count = 0;
  self->on_closed_done_mutex = NULL;

  if (self->on_closed_done_mode ==
      TEN_CLOSEABLE_ON_CLOSED_DONE_MODE_OUT_OWN_THREAD) {
    self->on_closed_done_mutex = ten_mutex_create();
  }

  ten_list_init(&self->on_intend_to_close_queue);
  ten_list_init(&self->on_closed_queue);
  ten_list_init(&self->on_closed_all_done_queue);
}

void ten_closeable_init(ten_closeable_t *self, ptrdiff_t offset) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_signature_set(&self->signature, (ten_signature_t)TEN_CLOSEABLE_SIGNATURE);
  ten_sanitizer_thread_check_init_with_current_thread(&self->thread_check);

  self->state = TEN_CLOSEABLE_STATE_ALIVE;
  self->offset_in_impl = offset;

  self->is_closing_root_myself_cb = NULL;
  self->is_closing_root_myself_data = NULL;

  ten_closeable_action_to_close_myself_init(&self->action_to_close_myself);

  ten_closeable_be_notified_resources_init(&self->be_notified_resources);

  ten_list_init(&self->belong_to_resources);
  ten_list_init(&self->be_depended_on_resources);
  ten_list_init(&self->underlying_resources);
}

static void ten_closeable_be_notified_resources_deinit(
    ten_closeable_be_notified_resources_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_list_clear(&self->on_intend_to_close_queue);
  ten_list_clear(&self->on_closed_queue);
  ten_list_clear(&self->on_closed_all_done_queue);

  if (self->on_closed_done_mutex) {
    ten_mutex_destroy(self->on_closed_done_mutex);
    self->on_closed_done_mutex = NULL;
  }
}

void ten_closeable_deinit(ten_closeable_t *self) {
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: The belonging thread (i.e., ten_thread_t object) maybe
  // already destroyed, so do not check thread integrity.
  TEN_ASSERT(self && ten_closeable_check_integrity(self, false),
             "Invalid argument.");

  ten_signature_set(&self->signature, 0);
  ten_sanitizer_thread_check_deinit(&self->thread_check);

  self->state = TEN_CLOSEABLE_STATE_ALIVE;

  ten_closeable_be_notified_resources_deinit(&self->be_notified_resources);

  ten_list_clear(&self->belong_to_resources);
  ten_list_clear(&self->be_depended_on_resources);
  ten_list_clear(&self->underlying_resources);
}

void ten_closeable_set_action_to_close_myself(
    ten_closeable_t *self,
    ten_closeable_action_to_close_myself_func_t action_to_close_myself_cb,
    void *action_to_close_myself_data) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  self->action_to_close_myself.action_to_close_myself_cb =
      action_to_close_myself_cb;
  self->action_to_close_myself.action_to_close_myself_data =
      action_to_close_myself_data;
}

void ten_closeable_set_is_closing_root_myself(
    ten_closeable_t *self,
    ten_closeable_is_closing_root_func_t is_closing_root_myself_cb,
    void *is_closing_root_myself_data) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Access across threads.");
  TEN_ASSERT(is_closing_root_myself_cb, "Invalid argument.");

  self->is_closing_root_myself_cb = is_closing_root_myself_cb;
  self->is_closing_root_myself_data = is_closing_root_myself_data;
}

static void ten_closeable_on_closed_all_done(ten_closeable_t *self) {
  TEN_ASSERT(self, "Should not happen.");

  bool is_thread_safe = self->be_notified_resources.on_closed_done_mode ==
                        TEN_CLOSEABLE_ON_CLOSED_DONE_MODE_IN_OWN_THREAD;
  TEN_ASSERT(ten_closeable_check_integrity(self, is_thread_safe),
             "Invalid argument.");

  ten_list_t on_closed_all_done_queue = TEN_LIST_INIT_VAL;

  // Because on_closed_done() may be invoked in different threads than the one
  // where ten_closeable_t resides, it is necessary to determine whether to
  // perform lock/unlock operations based on thread safety principles.
  if (is_thread_safe) {
    ten_list_swap(&on_closed_all_done_queue,
                  &self->be_notified_resources.on_closed_all_done_queue);
  } else {
    TEN_ASSERT(self->be_notified_resources.on_closed_done_mutex,
               "Should not happen.");

    TEN_DO_WITH_MUTEX_LOCK(self->be_notified_resources.on_closed_done_mutex, {
      ten_list_swap(&on_closed_all_done_queue,
                    &self->be_notified_resources.on_closed_all_done_queue);
    });
  }

  ten_list_foreach (&on_closed_all_done_queue, iter) {
    ten_closeable_on_closed_all_done_item_t *item =
        ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(item && item->on_closed_all_done_cb, "Should not happen.");

    item->on_closed_all_done_cb(self, item->who_have_interest_on_me,
                                item->on_closed_all_done_data);
  };

  ten_list_clear(&on_closed_all_done_queue);
}

static void ten_closeable_on_closed_done(ten_closeable_t *self,
                                         void *who_have_interest_on_me,
                                         void *on_closed_data) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  ten_list_t *on_closed_queue = &self->be_notified_resources.on_closed_queue;

  bool found = false;
  ten_list_foreach (on_closed_queue, iter) {
    ten_closeable_on_closed_item_t *item = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(item && item->on_closed_cb, "Should not happen.");

    if (item->who_have_interest_on_me == who_have_interest_on_me &&
        item->on_closed_data == on_closed_data) {
      found = true;
      ten_list_remove_node(on_closed_queue, iter.node);

      break;
    }
  }

  TEN_ASSERT(found, "Should not happen.");

  if (ten_list_is_empty(on_closed_queue)) {
    // All other resources who are interested in my 'closed' event handle the
    // 'closed' event completely.
    ten_closeable_on_closed_all_done(self);
  }
}

static void ten_closeable_on_closed_done_out_of_thread(
    ten_closeable_t *self, TEN_UNUSED void *who_have_interest_on_me,
    TEN_UNUSED void *on_closed_data) {
  // TEN_NOLINTNEXTLINE(thread-check)
  // thread-check: This function is always called in threads other than the
  // thread where ten_closeable_t resides.
  TEN_ASSERT(self && ten_closeable_check_integrity(self, false),
             "Invalid argument.");
  TEN_ASSERT(self->be_notified_resources.on_closed_done_mode ==
                 TEN_CLOSEABLE_ON_CLOSED_DONE_MODE_OUT_OWN_THREAD,
             "Should not happen.");

  size_t remain_on_closed_done_count = 0;
  TEN_DO_WITH_MUTEX_LOCK(self->be_notified_resources.on_closed_done_mutex, {
    remain_on_closed_done_count =
        --self->be_notified_resources.expected_on_closed_done_count;
  });

  if (remain_on_closed_done_count == 0) {
    ten_closeable_on_closed_all_done(self);
  }
}

static void ten_closeable_make_intend_to_close_announcement(
    ten_closeable_t *self) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  ten_list_foreach (&self->be_notified_resources.on_intend_to_close_queue,
                    iter) {
    ten_closeable_on_intend_to_close_item_t *item =
        ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(item, "Should not happen.");

    if (item->on_intend_to_close_cb) {
      item->on_intend_to_close_cb(self, item->who_have_interest_on_me,
                                  item->on_intend_to_close_data);
    }
  }
}

/**
 * @return true if @a self is the closing root, otherwise false.
 */
static bool ten_closeable_is_closing_root_myself(ten_closeable_t *self) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  // If 'is_closing_root_myself_cb' is provided, it means the 'containing'
  // instance has its own checking logic, rather than using the default behavior
  // which search the closing roots through belong_to_resources.
  if (self->is_closing_root_myself_cb) {
    return self->is_closing_root_myself_cb(self, NULL,
                                           self->is_closing_root_myself_data);
  }

  // The following is the default behavior.

  bool I_am_root = true;

  ten_list_foreach (&self->belong_to_resources, iter) {
    ten_closeable_belong_to_info_t *belong_to_info =
        ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(belong_to_info, "Should not happen.");
    TEN_ASSERT(ten_closeable_check_integrity(belong_to_info->belong_to, true) &&
                   belong_to_info->is_closing_root_cb,
               "Should not happen.");

    if (belong_to_info->is_closing_root_cb(
            belong_to_info->belong_to, self,
            belong_to_info->is_closing_root_data) == true) {
      // Find a new root, so I am not the root.
      I_am_root = false;

      break;
    }
  }

  return I_am_root;
}

void ten_closeable_intend_to_close(ten_closeable_t *self,
                                   TEN_UNUSED void *intend_to_close_data) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  if (self->state >= TEN_CLOSEABLE_STATE_CLOSING) {
    return;
  }

  // Make an announcement first.
  ten_closeable_make_intend_to_close_announcement(self);

  // Determine if I am the closing root.
  bool I_am_root = ten_closeable_is_closing_root_myself(self);
  if (I_am_root) {
    // I am the closing root, so trigger the closing directly.
    ten_closeable_close(self);
  }
}

static bool ten_closeable_could_start_to_close_myself(ten_closeable_t *self) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  // Check if all remaining underlying_resources have been closed.
  ten_list_foreach (&self->underlying_resources, iter) {
    ten_closeable_t *target = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(target && ten_closeable_check_integrity(target, true),
               "Should not happen.");

    if (!ten_closeable_is_closed(target)) {
      return false;
    }
  }

  // Check if all remaining depend_resources have been closed.
  ten_list_foreach (&self->be_depended_on_resources, iter) {
    ten_closeable_t *target = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(target && ten_closeable_check_integrity(target, true),
               "Should not happen.");

    if (!ten_closeable_is_closed(target)) {
      return false;
    }
  }

  return true;
}

void ten_closeable_action_to_close_myself_done(
    ten_closeable_t *self, TEN_UNUSED void *on_close_myself_data) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  self->state = TEN_CLOSEABLE_STATE_CLOSED;

  if (ten_list_is_empty(&self->be_notified_resources.on_closed_queue)) {
    ten_closeable_on_closed_all_done(self);
    return;
  }

  // Under different modes, the logic for determining the receipt of all
  // on_closed_done() is different.
  if (self->be_notified_resources.on_closed_done_mode ==
      TEN_CLOSEABLE_ON_CLOSED_DONE_MODE_OUT_OWN_THREAD) {
    self->be_notified_resources.expected_on_closed_done_count =
        ten_list_size(&self->be_notified_resources.on_closed_queue);

    self->be_notified_resources.on_closed_done_cb =
        ten_closeable_on_closed_done_out_of_thread;
  } else {
    self->be_notified_resources.on_closed_done_cb =
        ten_closeable_on_closed_done;
  }

  // Notify others who are interested in my 'closed' event.
  ten_list_foreach (&self->be_notified_resources.on_closed_queue, iter) {
    ten_closeable_on_closed_item_t *item = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(item && item->on_closed_cb, "Should not happen.");

    item->on_closed_cb(self, item->who_have_interest_on_me,
                       item->on_closed_data,
                       self->be_notified_resources.on_closed_done_cb);
  }
}

/**
 * @brief All the underlying resources are closed, so I could start to close
 * myself.
 */
static void ten_closeable_do_close(ten_closeable_t *self) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  if (self->action_to_close_myself.action_to_close_myself_cb) {
    self->action_to_close_myself.action_to_close_myself_cb(
        self, self->action_to_close_myself.action_to_close_myself_data,
        ten_closeable_action_to_close_myself_done);
  } else {
    // There is no customized close_myself actions, so call
    // 'action_to_close_myself_done' directly.
    ten_closeable_action_to_close_myself_done(self, NULL);
  }
}

static void ten_closeable_on_underlying_resource_closed_default(
    ten_closeable_t *underlying_resource, void *self_, void *on_closed_data,
    ten_closeable_on_closed_done_func_t on_closed_done) {
  TEN_ASSERT(underlying_resource &&
                 ten_closeable_check_integrity(underlying_resource, true),
             "Invalid argument.");

  ten_closeable_t *self = (ten_closeable_t *)self_;
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  ten_list_remove_ptr(&self->underlying_resources, underlying_resource);

  if (self->state == TEN_CLOSEABLE_STATE_CLOSING) {
    if (ten_closeable_could_start_to_close_myself(self)) {
      ten_closeable_do_close(self);
    }
  }

  // Notify my underlying resource that I have received its 'closed' event and
  // have completed all the tasks I needed to do.
  if (on_closed_done) {
    on_closed_done(underlying_resource, self, on_closed_data);
  }
}

static void ten_closeable_on_intend_to_close_item_destroy(
    ten_closeable_on_intend_to_close_item_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  TEN_FREE(self);
}

static void ten_closeable_on_closed_item_destroy(
    ten_closeable_on_closed_item_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  TEN_FREE(self);
}

static void ten_closeable_on_closed_all_done_item_destroy(
    ten_closeable_on_closed_all_done_item_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  TEN_FREE(self);
}

void ten_closeable_add_be_notified(
    ten_closeable_t *self, void *who_have_interest_on_me,
    // intend_to_close event.
    ten_closeable_on_intend_to_close_func_t on_intend_to_close_cb,
    void *on_intend_to_close_data,
    // on_closed event.
    ten_closeable_on_closed_func_t on_closed_cb, void *on_closed_data,
    ten_closeable_on_closed_all_done_func_t on_closed_all_done_cb,
    void *on_closed_all_done_data) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  if (on_intend_to_close_cb) {
    ten_closeable_on_intend_to_close_item_t *item =
        TEN_MALLOC(sizeof(ten_closeable_on_intend_to_close_item_t));
    TEN_ASSERT(item, "Failed to allocate memory.");

    item->who_have_interest_on_me = who_have_interest_on_me;
    item->on_intend_to_close_cb = on_intend_to_close_cb;
    item->on_intend_to_close_data = on_intend_to_close_data;

    ten_list_push_ptr_back(
        &self->be_notified_resources.on_intend_to_close_queue, item,
        (ten_ptr_listnode_destroy_func_t)
            ten_closeable_on_intend_to_close_item_destroy);
  }

  if (on_closed_cb) {
    ten_closeable_on_closed_item_t *item =
        TEN_MALLOC(sizeof(ten_closeable_on_closed_item_t));
    TEN_ASSERT(item, "Failed to allocate memory.");

    item->who_have_interest_on_me = who_have_interest_on_me;
    item->on_closed_cb = on_closed_cb;
    item->on_closed_data = on_closed_data;

    ten_list_push_ptr_back(
        &self->be_notified_resources.on_closed_queue, item,
        (ten_ptr_listnode_destroy_func_t)ten_closeable_on_closed_item_destroy);
  }

  if (on_closed_all_done_cb) {
    ten_closeable_on_closed_all_done_item_t *item =
        TEN_MALLOC(sizeof(ten_closeable_on_closed_all_done_item_t));
    TEN_ASSERT(item, "Failed to allocate memory.");

    item->who_have_interest_on_me = who_have_interest_on_me;
    item->on_closed_all_done_cb = on_closed_all_done_cb;
    item->on_closed_all_done_data = on_closed_all_done_data;

    ten_list_push_ptr_back(
        &self->be_notified_resources.on_closed_all_done_queue, item,
        (ten_ptr_listnode_destroy_func_t)
            ten_closeable_on_closed_all_done_item_destroy);
  }
}

static void ten_closeable_on_depended_resource_closed_default(
    ten_closeable_t *self, void *depend, void *on_closed_data,
    ten_closeable_on_closed_done_func_t on_closed_done) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Access across threads.");

  ten_closeable_t *depend_ = (ten_closeable_t *)depend;
  TEN_ASSERT(depend_ && ten_closeable_check_integrity(depend_, true),
             "Access across threads.");

  // Remove @a self from the 'be_depended_on_resources' queue of @a depend_.
  ten_closeable_remove_depend_resource(self, depend_);

  // Note that @a self might be destroyed by its owner after the call to @a
  // on_closed_done.
  if (on_closed_done) {
    on_closed_done(self, depend, on_closed_data);
  }
}

void ten_closeable_add_depend_resource(
    ten_closeable_t *self, ten_closeable_t *depend,
    ten_closeable_on_closed_func_t on_closed_cb, void *on_closed_data,
    ten_closeable_on_intend_to_close_func_t on_intend_to_close_cb,
    void *on_intend_to_close_data) {
  ten_list_push_ptr_back(&depend->be_depended_on_resources, self, NULL);

  // I will receive the 'intend_to_close' event of the depend resource. I maybe
  // not able to keep running if my dependency is closed. This is an opportunity
  // for the closing tree, I belong to, to start the closing process.
  if (on_intend_to_close_cb) {
    ten_closeable_add_be_notified(depend, self, on_intend_to_close_cb,
                                  on_intend_to_close_data, NULL, NULL, NULL,
                                  NULL);
  } else {
    // I and my dependency are in the different closing trees, which means we
    // will be closed by our owners. So if someone depends on me, but he does
    // not want to care about my 'on_intend_to_close' event, we should not add a
    // default behavior such as 'intend_to_close' him when I am closing.
  }

  // My dependency should receive my 'on_closed' event, then my dependency could
  // be able to close itself if it is being closed.
  //
  // We have to add a default behavior to do some cleaning if the 'on_closed_cb'
  // is NULL.
  ten_closeable_add_be_notified(
      self, depend, NULL, NULL,
      on_closed_cb ? on_closed_cb
                   : ten_closeable_on_depended_resource_closed_default,
      on_closed_data, NULL, NULL);
}

void ten_closeable_remove_be_notified(ten_closeable_t *self,
                                      void *who_have_interest_on_me) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Access across threads.");

  ten_list_foreach (&self->be_notified_resources.on_closed_all_done_queue,
                    iter) {
    ten_closeable_on_closed_all_done_item_t *item =
        ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(item, "Should not happen.");

    if (item->who_have_interest_on_me == who_have_interest_on_me) {
      ten_list_remove_node(
          &self->be_notified_resources.on_closed_all_done_queue, iter.node);
    }
  }

  ten_list_foreach (&self->be_notified_resources.on_closed_queue, iter) {
    ten_closeable_on_closed_item_t *item = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(item, "Should not happen.");

    if (item->who_have_interest_on_me == who_have_interest_on_me) {
      ten_list_remove_node(&self->be_notified_resources.on_closed_queue,
                           iter.node);
    }
  }

  ten_list_foreach (&self->be_notified_resources.on_intend_to_close_queue,
                    iter) {
    ten_closeable_on_intend_to_close_item_t *item =
        ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(item, "Should not happen.");

    if (item->who_have_interest_on_me == who_have_interest_on_me) {
      ten_list_remove_node(
          &self->be_notified_resources.on_intend_to_close_queue, iter.node);
    }
  }
}

void ten_closeable_remove_depend_resource(ten_closeable_t *self,
                                          ten_closeable_t *depend_resource) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Access across threads.");
  TEN_ASSERT(
      depend_resource && ten_closeable_check_integrity(depend_resource, true),
      "Access across threads.");

  // Delete 'self' from 'be_depended_on_resources' of 'depend_resource'.
  ten_list_remove_ptr(&depend_resource->be_depended_on_resources, self);

  // Remove all the events that @a depend_resource subscribes on @a self.
  ten_closeable_remove_be_notified(depend_resource, self);

  if (depend_resource->state == TEN_CLOSEABLE_STATE_CLOSING) {
    if (ten_closeable_could_start_to_close_myself(depend_resource)) {
      ten_closeable_do_close(depend_resource);
    }
  }
}

void ten_closeable_remove_depend_resource_bidirectional(
    ten_closeable_t *self, ten_closeable_t *depend_resource) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Access across threads.");
  TEN_ASSERT(
      depend_resource && ten_closeable_check_integrity(depend_resource, true),
      "Access across threads.");

  ten_closeable_remove_be_notified(self, depend_resource);
  ten_closeable_remove_depend_resource(self, depend_resource);
}

static void ten_closeable_belong_to_info_destroy(
    ten_closeable_belong_to_info_t *info) {
  TEN_ASSERT(info, "Invalid argument.");

  TEN_FREE(info);
}

static bool ten_closeable_is_closing_root_default(
    ten_closeable_t *self, TEN_UNUSED ten_closeable_t *underlying_resource,
    TEN_UNUSED void *is_closing_root_data) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(underlying_resource &&
                 ten_closeable_check_integrity(underlying_resource, true),
             "Invalid argument.");

  // The default behavior is whether 'self' is a root is determined by whether
  // it has an owner.
  return ten_list_is_empty(&self->belong_to_resources);
}

static void ten_closeable_add_belong_to_resource(
    ten_closeable_t *self, ten_closeable_t *belong_to,
    ten_closeable_is_closing_root_func_t is_closing_root_cb,
    void *is_closing_root_data) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(belong_to && ten_closeable_check_integrity(belong_to, true),
             "Invalid argument.");

  ten_closeable_belong_to_info_t *info =
      (ten_closeable_belong_to_info_t *)TEN_MALLOC(
          sizeof(ten_closeable_belong_to_info_t));
  TEN_ASSERT(info, "Failed to allocate memory.");

  info->belong_to = belong_to;

  info->is_closing_root_cb = is_closing_root_cb
                                 ? is_closing_root_cb
                                 : ten_closeable_is_closing_root_default;
  info->is_closing_root_data = is_closing_root_data;

  ten_list_push_ptr_back(
      &self->belong_to_resources, info,
      (ten_ptr_listnode_destroy_func_t)ten_closeable_belong_to_info_destroy);
}

void ten_closeable_add_underlying_resource(
    ten_closeable_t *self, ten_closeable_t *underlying_resource,
    ten_closeable_is_closing_root_func_t is_closing_root_cb,
    void *is_closing_root_data,
    ten_closeable_on_intend_to_close_func_t on_intend_to_close_cb,
    void *on_intend_to_close_data,
    ten_closeable_on_closed_all_done_func_t on_closed_all_done_cb,
    void *on_closed_all_done_data) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Access across threads.");
  TEN_ASSERT(underlying_resource &&
                 ten_closeable_check_integrity(underlying_resource, true),
             "Access across threads.");
  TEN_ASSERT(on_closed_all_done_cb,
             "The owner must subscribe the 'on_close_all_done' events of its "
             "underlying resources.");

  ten_list_push_ptr_back(&self->underlying_resources, underlying_resource,
                         NULL);

  // I should receive the closed event from 'resource'.
  ten_closeable_add_be_notified(
      underlying_resource, self, on_intend_to_close_cb, on_intend_to_close_data,
      ten_closeable_on_underlying_resource_closed_default, NULL,
      on_closed_all_done_cb, on_closed_all_done_data);

  ten_closeable_add_belong_to_resource(
      underlying_resource, self, is_closing_root_cb, is_closing_root_data);
}

void ten_closeable_close(ten_closeable_t *self) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");
  TEN_ASSERT(self->state < TEN_CLOSEABLE_STATE_CLOSING,
             "The 'close()' function could be called only once to ensure that "
             "all the resources are closed from top to bottom.");

  self->state = TEN_CLOSEABLE_STATE_CLOSING;

  if (ten_closeable_could_start_to_close_myself(self)) {
    ten_closeable_do_close(self);
  } else {
    ten_list_foreach (&self->underlying_resources, iter) {
      ten_closeable_t *resource = ten_ptr_listnode_get(iter.node);
      ten_closeable_close(resource);
    }
  }
}

bool ten_closeable_is_closed(ten_closeable_t *self) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  return self->state == TEN_CLOSEABLE_STATE_CLOSED;
}

bool ten_closeable_is_closing(ten_closeable_t *self) {
  TEN_ASSERT(self && ten_closeable_check_integrity(self, true),
             "Invalid argument.");

  return self->state == TEN_CLOSEABLE_STATE_CLOSING;
}
