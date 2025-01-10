//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/metadata/metadata.h"
#include "axis_utils/lib/signature.h"
#include "axis_utils/lib/string.h"

#define axis_METADATA_INFO_SIGNATURE 0xE3E7657449860D3BU
#define axis_METADATA_INFOS_SIGNATURE 0x6C5D75C01FD2CBBFU

typedef struct axis_env_t axis_env_t;

typedef void (*axis_metadata_info_destroy_handler_in_target_lang_func_t)(
    void *me_in_target_lang);

typedef enum axis_METADATA_ATTACH_TO {
  axis_METADATA_ATTACH_TO_INVALID,

  axis_METADATA_ATTACH_TO_MANIFEST,
  axis_METADATA_ATTACH_TO_PROPERTY,
} axis_METADATA_ATTACH_TO;

typedef struct axis_metadata_info_t {
  axis_signature_t signature;

  axis_METADATA_ATTACH_TO attach_to;

  axis_METADATA_TYPE type;
  axis_string_t *value;

  // The object (i.e., addon/extension/extension_group/app) that this metadata
  // belongs to. Because the `value` might be a relative path to the base_dir of
  // the belonging object, and we need to check if it is valid in
  // `axis_metadata_info_set()` before `on_init_done()`, so we need to remember
  // this to get the base_dir.
  axis_env_t *belonging_to;
} axis_metadata_info_t;

axis_RUNTIME_API bool axis_metadata_info_check_integrity(
    axis_metadata_info_t *self);

axis_RUNTIME_PRIVATE_API axis_metadata_info_t *axis_metadata_info_create(
    axis_METADATA_ATTACH_TO attach_to, axis_env_t *belonging_to);
