//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/engine/internal/thread.h"

#include <stdbool.h>
#include <stdlib.h>

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/connection/connection.h"
#include "include_internal/ten_runtime/engine/engine.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/app/app.h"
#include "ten_utils/io/runloop.h"
#include "ten_utils/lib/event.h"
#include "ten_utils/lib/string.h"
#include "ten_utils/lib/thread.h"
#include "ten_utils/macro/check.h"
#include "ten_utils/macro/mark.h"
#include "ten_utils/sanitizer/thread_check.h"

#define TIMEOUT_FOR_ENGINE_THREAD_STARTED 5000  // ms

static void *ten_engine_thread_main(void *self_) {
  ten_engine_t *self = (ten_engine_t *)self_;
  TEN_ASSERT(self, "Should not happen.");

  ten_sanitizer_thread_check_set_belonging_thread(&self->thread_check, NULL);

  ten_event_wait(self->belonging_thread_is_set, -1);
  TEN_ASSERT(ten_engine_check_integrity(self, true), "Should not happen.");

  TEN_LOGD(
      "[%s] Engine thread %p is started.", ten_app_get_uri(self->app),
      ten_sanitizer_thread_check_get_belonging_thread(&self->thread_check));

  // Because the path table is created in the original thread (ex: the app
  // thread), so we need to correct the belonging_thread of the path table now.
  ten_sanitizer_thread_check_set_belonging_thread_to_current_thread(
      &self->path_table->thread_check);

  TEN_LOGD("[%s] Engine thread is started.", ten_app_get_uri(self->app));

  // Create an eventloop dedicated to the engine.
  self->loop = ten_runloop_create(NULL);

  // Create the ten_env for the engine on the engine thread.
  TEN_ASSERT(!self->ten_env, "Should not happen.");
  self->ten_env = ten_env_create_for_engine(self);

  // Notify that the engine thread is started, and the mechanism related to the
  // pipe reading has been established, and could start to receive the file
  // descriptor of a channel from the pipe.
  ten_event_set(self->engine_thread_ready_for_migration);

  // This is a blocking call, until the engine is about to close.
  ten_runloop_run(self->loop);

  if (self->on_closed) {
    // The engine is closing, call the registered on_close callback if exists.
    self->on_closed(self, self->on_closed_data);
  }

  TEN_LOGD("[%s] Engine thread is stopped", ten_app_get_uri(self->app));

  return NULL;
}

void ten_engine_create_its_own_thread(ten_engine_t *self) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(ten_app_check_integrity(self->app, true), "Should not happen.");

  self->belonging_thread_is_set = ten_event_create(0, 0);
  self->engine_thread_ready_for_migration = ten_event_create(0, 0);

  ten_thread_create(ten_string_get_raw_str(&self->graph_id),
                    ten_engine_thread_main, self);

  ten_event_set(self->belonging_thread_is_set);

  // Wait the engine thread been started, so that we can transfer the fd of
  // 'connection->stream' to it.
  TEN_UNUSED int rc = ten_event_wait(self->engine_thread_ready_for_migration,
                                     TIMEOUT_FOR_ENGINE_THREAD_STARTED);
  TEN_ASSERT(!rc, "Should not happen.");
}

void ten_engine_init_individual_eventloop_relevant_vars(ten_engine_t *self,
                                                        ten_app_t *app) {
  TEN_ASSERT(self && ten_engine_check_integrity(self, true),
             "Should not happen.");
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Should not happen.");

  // Each engine can have its own decision of having its own eventloop. We
  // currently use a simplified strategy to determine this decision of every
  // engines. If we have a more complex policy decision in the future, just
  // modify the following line is enough.
  if (app->one_event_loop_per_engine) {
    self->has_own_loop = true;
  } else {
    self->has_own_loop = false;
  }
}
