//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/ten_config.h"

#include "ten_runtime/ten_env_proxy/ten_env_proxy.h"

// =-=-= 改名
typedef struct ten_extension_test_t {
  ten_thread_t *test_app_thread;
  ten_string_t test_extension_addon_name;
  ten_env_proxy_t *test_app_ten_env_proxy;
  ten_event_t *test_app_ten_env_proxy_create_completed;
} ten_extension_test_t;

TEN_RUNTIME_API ten_extension_test_t *ten_extension_test_create(void);

TEN_RUNTIME_API void ten_extension_test_destroy(ten_extension_test_t *self);

TEN_RUNTIME_API void ten_extension_test_start(ten_extension_test_t *self);

TEN_RUNTIME_API void ten_extension_test_add_addon(ten_extension_test_t *self,
                                                  const char *addon_name);
