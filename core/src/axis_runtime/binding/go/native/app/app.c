//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_runtime/binding/go/interface/ten/app.h"

#include <stdlib.h>

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/binding/go/app/app.h"
#include "include_internal/axis_runtime/binding/go/internal/common.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env.h"
#include "include_internal/axis_runtime/binding/go/axis_env/axis_env_internal.h"
#include "include_internal/axis_runtime/global/signal.h"
#include "axis_runtime/binding/common.h"
#include "axis_runtime/binding/go/interface/ten/common.h"
#include "axis_runtime/binding/go/interface/ten/axis_env.h"
#include "axis_runtime/ten.h"
#include "axis_runtime/axis_env_proxy/axis_env_proxy.h"
#include "axis_utils/lib/alloc.h"

void tenGoAppOnConfigure(axis_go_handle_t go_app, axis_go_handle_t go_axis_env);

void tenGoAppOnInit(axis_go_handle_t go_app, axis_go_handle_t go_axis_env);

void tenGoAppOnDeinit(axis_go_handle_t go_app, axis_go_handle_t go_axis_env);

bool axis_go_app_check_integrity(axis_go_app_t *self) {
  axis_ASSERT(self, "Should not happen.");

  if (axis_signature_get(&self->signature) != axis_GO_APP_SIGNATURE) {
    return false;
  }

  return true;
}

static void proxy_on_configure(axis_app_t *app, axis_env_t *axis_env) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_app_get_axis_env(app) == axis_env, "Should not happen.");

  axis_go_app_t *app_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)app);
  axis_ASSERT(app_bridge, "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);
  axis_env_bridge->c_axis_env_proxy = axis_env_proxy_create(axis_env, 1, NULL);

  tenGoAppOnConfigure(app_bridge->bridge.go_instance,
                      axis_go_axis_env_go_handle(axis_env_bridge));
}

static void proxy_on_init(axis_app_t *app, axis_env_t *axis_env) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");
  axis_ASSERT(axis_env && axis_env_check_integrity(axis_env, true),
             "Should not happen.");
  axis_ASSERT(axis_app_get_axis_env(app) == axis_env, "Should not happen.");

  axis_go_app_t *app_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)app);
  axis_ASSERT(app_bridge, "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

  tenGoAppOnInit(app_bridge->bridge.go_instance,
                 axis_go_axis_env_go_handle(axis_env_bridge));
}

static void proxy_on_deinit(axis_app_t *app, axis_env_t *axis_env) {
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");

  axis_go_app_t *app_bridge =
      axis_binding_handle_get_me_in_target_lang((axis_binding_handle_t *)app);
  axis_ASSERT(app_bridge, "Should not happen.");

  axis_go_axis_env_t *axis_env_bridge = axis_go_axis_env_wrap(axis_env);

  tenGoAppOnDeinit(app_bridge->bridge.go_instance,
                   axis_go_axis_env_go_handle(axis_env_bridge));
}

static void axis_go_app_destroy(axis_go_app_t *self) {
  axis_ASSERT(self && axis_go_app_check_integrity(self), "Should not happen.");

  axis_app_destroy(self->c_app);
  axis_FREE(self);
}

axis_go_app_t *axis_go_app_create(axis_go_handle_t go_app_index) {
  axis_go_app_t *app_bridge = (axis_go_app_t *)axis_MALLOC(sizeof(axis_go_app_t));
  axis_ASSERT(app_bridge, "Failed to allocate memory.");

  axis_signature_set(&app_bridge->signature, axis_GO_APP_SIGNATURE);
  app_bridge->bridge.go_instance = go_app_index;

  // The app bridge instance is created and managed only by Go. When the Go app
  // is finalized, the app bridge instance will be destroyed. Therefore, the C
  // part should not hold any reference to the app bridge instance.
  app_bridge->bridge.sp_ref_by_go =
      axis_shared_ptr_create(app_bridge, axis_go_app_destroy);
  app_bridge->bridge.sp_ref_by_c = NULL;

  app_bridge->c_app =
      axis_app_create(proxy_on_configure, proxy_on_init, proxy_on_deinit, NULL);
  axis_binding_handle_set_me_in_target_lang(
      (axis_binding_handle_t *)(app_bridge->c_app), app_bridge);

  // Setup the default signal handler for GO app. The reason for setting up the
  // signal handler for GO app is as follows.
  //
  // 1. Because of the linked mechanism, the following function
  // `axis_global_setup_signal_stuff()` will be called after the GO process is
  // created, and before the GO runtime is initialized. Refer to
  // `axis_CONSTRUCTOR`.
  //
  // 2. Then the GO runtime starts, and a default signal handler is set up in
  // the GO world. The `sigaction` function is called from GO using cgo, and the
  // handler setup by `axis_global_setup_signal_stuff()` in the above step is
  // replaced.
  //
  // The code snippet of handling the `SIGINT` and `SIGTERM` in GO runtime is as
  // follows.
  //
  //   /* Setup the default signal handler in GO.*/
  //   func setsig(i uint32, fn uintptr) {
  //     var sa sigactiont
  //     sa.sa_flags = _SA_SIGINFO | _SA_ONSTACK | _SA_RESTORER | _SA_RESTART
  //     if GOARCH == "386" || GOARCH == "amd64" {
  //       sa.sa_restorer = abi.FuncPCABI0(sigreturn__sigaction)
  //     }
  //
  //     sigaction(i, &sa, nil)
  //   }
  //
  //   /* The default signal handler in GO. */
  //   func sigfwdgo(sig uint32, ...) bool {
  //     /* We are not handling the signal and there is no other handler to */
  //     /* forward to. Crash with the default behavior. */
  // 		 if fwdFn == _SIG_DFL {
  // 			 setsig(sig, _SIG_DFL)
  // 			 dieFromSignal(sig)
  // 			 return false
  // 		 }
  //
  // 		 sigfwd(fwdFn, sig, info, ctx)
  // 		 return true
  //   }
  //
  // In brief, the GO runtime will setup a default signal handler, and save the
  // old handler (i.e., axis_global_signal_handler). When GO runtime receives a
  // `SIGINT` or `SIGTERM`, it forwards the signal to the old handler first, and
  // then crashes the process as SIGKILL.
  //
  // However, the signal handler (i.e., axis_global_signal_handler) is an async
  // function, which means after the handler returns, the TEN app may not be
  // closed completely yet, and the `on_stop` or `on_deinit` callback of
  // extensions may not be called.
  //
  // 3. After the GO runtime starts, the GO `main` function will be called. Any
  // `sigaction` function called in the `main` function will replace the default
  // signal handler setup by the GO runtime. Ex: the following function
  // axis_global_setup_signal_stuff().

  axis_global_setup_signal_stuff();

  return app_bridge;
}

void axis_go_app_run(axis_go_app_t *app_bridge, bool run_in_background) {
  axis_ASSERT(app_bridge && axis_go_app_check_integrity(app_bridge),
             "Should not happen.");

  axis_app_run(app_bridge->c_app, run_in_background, NULL);
}

void axis_go_app_close(axis_go_app_t *app_bridge) {
  axis_ASSERT(app_bridge && axis_go_app_check_integrity(app_bridge),
             "Should not happen.");

  axis_app_close(app_bridge->c_app, NULL);
}

void axis_go_app_wait(axis_go_app_t *app_bridge) {
  axis_ASSERT(app_bridge && axis_go_app_check_integrity(app_bridge),
             "Should not happen.");

  axis_app_wait(app_bridge->c_app, NULL);
}

void axis_go_app_finalize(axis_go_app_t *self) {
  // The Go app is finalized, it's time to destroy the C app.
  axis_ASSERT(self && axis_go_app_check_integrity(self), "Should not happen.");

  axis_go_bridge_destroy_go_part(&self->bridge);
}
