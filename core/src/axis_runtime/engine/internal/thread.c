//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/engine/internal/thread.h"

#include <stdbool.h>
#include <stdlib.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/engine/engine.h"
#include "include_internal/axis_runtime/axis_env/axis_env.h"
#include "axis_runtime/app/app.h"
#include "axis_utils/io/runloop.h"
#include "axis_utils/lib/event.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/lib/thread.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"
#include "axis_utils/sanitizer/thread_check.h"

#define TIMEOUT_FOR_ENGINE_THREAD_STARTED 5000  // ms

static void *axis_engine_thread_main(void *self_) {
  axis_engine_t *self = (axis_engine_t *)self_;
  axis_ASSERT(self, "Should not happen.");

  axis_sanitizer_thread_check_set_belonging_thread(&self->thread_check, NULL);

  axis_event_wait(self->belonging_thread_is_set, -1);
  axis_ASSERT(axis_engine_check_integrity(self, true), "Should not happen.");

  axis_LOGD(
      "[%s] Engine thread %p is started.", axis_app_get_uri(self->app),
      axis_sanitizer_thread_check_get_belonging_thread(&self->thread_check));

  // Because the path table is created in the original thread (ex: the app
  // thread), so we need to correct the belonging_thread of the path table now.
  axis_sanitizer_thread_check_set_belonging_thread_to_current_thread(
      &self->path_table->thread_check);

  axis_LOGD("[%s] Engine thread is started.", axis_app_get_uri(self->app));

  // Create an eventloop dedicated to the engine.
  self->loop = axis_runloop_create(NULL);

  // Create the axis_env for the engine on the engine thread.
  axis_ASSERT(!self->axis_env, "Should not happen.");
  self->axis_env = axis_env_create_for_engine(self);

  // Notify that the engine thread is started, and the mechanism related to the
  // pipe reading has been established, and could start to receive the file
  // descriptor of a channel from the pipe.
  axis_event_set(self->engine_thread_ready_for_migration);

  // This is a blocking call, until the engine is about to close.
  axis_runloop_run(self->loop);

  if (self->on_closed) {
    // The engine is closing, call the registered on_close callback if exists.
    self->on_closed(self, self->on_closed_data);
  }

  axis_LOGD("[%s] Engine thread is stopped", axis_app_get_uri(self->app));

  return NULL;
}

void axis_engine_create_its_own_thread(axis_engine_t *self) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(axis_app_check_integrity(self->app, true), "Should not happen.");

  self->belonging_thread_is_set = axis_event_create(0, 0);
  self->engine_thread_ready_for_migration = axis_event_create(0, 0);

  axis_thread_create(axis_string_get_raw_str(&self->graph_id),
                    axis_engine_thread_main, self);

  axis_event_set(self->belonging_thread_is_set);

  // Wait the engine thread been started, so that we can transfer the fd of
  // 'connection->stream' to it.
  axis_UNUSED int rc = axis_event_wait(self->engine_thread_ready_for_migration,
                                     TIMEOUT_FOR_ENGINE_THREAD_STARTED);
  axis_ASSERT(!rc, "Should not happen.");
}

void axis_engine_init_individual_eventloop_relevant_vars(axis_engine_t *self,
                                                        axis_app_t *app) {
  axis_ASSERT(self && axis_engine_check_integrity(self, true),
             "Should not happen.");
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");

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
