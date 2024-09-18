//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/test/extension_test.h"

#include <string.h>

#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/extension_thread/extension_thread.h"
#include "include_internal/ten_runtime/extension_thread/on_xxx.h"
#include "ten_runtime/extension/extension.h"
#include "ten_utils/container/list.h"
#include "ten_utils/container/list_ptr.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/macro/memory.h"

static void *ten_extension_thread_main(void *self_) {
  ten_extension_test_t *self = (ten_extension_test_t *)self_;
  TEN_ASSERT(self, "Should not happen.");

  ten_list_foreach (&self->pre_created_extensions, iter) {
    ten_extension_t *pre_created_extension = ten_ptr_listnode_get(iter.node);
    TEN_ASSERT(pre_created_extension, "Should not happen.");

    ten_extension_inherit_thread_ownership(pre_created_extension,
                                           self->test_extension_thread);

    pre_created_extension->extension_thread = self->test_extension_thread;
    ten_extension_set_unique_name_in_graph(pre_created_extension);
  }

  return ten_extension_thread_main_actual(self->test_extension_thread);
}

ten_extension_test_t *ten_extension_test_create(
    ten_extension_t *test_extension, ten_extension_t *target_extension) {
  ten_extension_test_t *self = TEN_MALLOC(sizeof(ten_extension_test_t));
  TEN_ASSERT(self, "Failed to allocate memory.");

  self->test_extension_thread = ten_extension_thread_create();
  self->test_extension_group = ten_extension_group_create_internal(
      "test_extension_group", NULL, NULL, NULL, NULL, NULL);

  ten_list_init(&self->pre_created_extensions);
  ten_list_push_ptr_back(&self->pre_created_extensions, test_extension, NULL);
  ten_list_push_ptr_back(&self->pre_created_extensions, target_extension, NULL);

  ten_extension_direct_all_msg_to_another_extension(test_extension,
                                                    target_extension);
  ten_extension_direct_all_msg_to_another_extension(test_extension,
                                                    target_extension);

  self->test_thread =
      ten_thread_create("extension thread", ten_extension_thread_main, self);

  return self;
}

void ten_extension_test_wait(ten_extension_test_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  int rc = ten_thread_join(self->test_thread, -1);
  TEN_ASSERT(!rc, "Should not happen.");
}

void ten_extension_test_destroy(ten_extension_test_t *self) {
  TEN_ASSERT(self, "Invalid argument.");

  ten_extension_thread_destroy(self->test_extension_thread);

  TEN_FREE(self);
}
