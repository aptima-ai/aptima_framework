//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/extension/extension.h"
#include "include_internal/ten_runtime/extension_group/extension_group.h"
#include "include_internal/ten_runtime/ten_env/ten_env.h"
#include "ten_runtime/ten_env/ten_env.h"
#include "ten_utils/lib/string.h"

const char *ten_env_get_attached_instance_name(ten_env_t *self,
                                               bool check_thread) {
  TEN_ASSERT(self && ten_env_check_integrity(self, check_thread),
             "Invalid argument.");

  switch (self->attach_to) {
    case TEN_ENV_ATTACH_TO_EXTENSION:
      return ten_string_get_raw_str(&self->attached_target.extension->name);

    case TEN_ENV_ATTACH_TO_EXTENSION_GROUP:
      return ten_string_get_raw_str(
          &self->attached_target.extension_group->name);

    case TEN_ENV_ATTACH_TO_APP:
      return ten_string_get_raw_str(&self->attached_target.app->uri);

    case TEN_ENV_ATTACH_TO_ADDON:
      return ten_string_get_raw_str(&self->attached_target.addon_host->name);

    default:
      TEN_ASSERT(0, "Handle more types: %d", self->attach_to);
      return NULL;
  }
}
