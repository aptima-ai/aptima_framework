//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/extension/extension.h"
#include "ten_utils/lib/signature.h"
#include "ten_utils/lib/smart_ptr.h"
#include "ten_utils/sanitizer/thread_check.h"

#define TEN_EXTENSIONHDR_SIGNATURE 0xE0836757A0BC0DFAU

// The extension struct and extension_info struct both represent an extension.
// For each extension, the extension_info is always present. If a certain
// extension is in the current process, then in addition to extension_info,
// there will also be an extension struct instance representing that extension.
typedef enum TEN_EXTENSION_TYPE {
  TEN_EXTENSION_TYPE_EXTENSION_INFO,
  TEN_EXTENSION_TYPE_EXTENSION,
} TEN_EXTENSION_TYPE;

typedef struct ten_extensionhdr_t {
  ten_signature_t signature;
  ten_sanitizer_thread_check_t thread_check;

  TEN_EXTENSION_TYPE type;
  union {
    ten_extension_t *extension;
    ten_weak_ptr_t *extension_info;
  } u;
} ten_extensionhdr_t;

TEN_RUNTIME_PRIVATE_API bool ten_extensionhdr_check_integrity(
    ten_extensionhdr_t *self, bool check_thread);

TEN_RUNTIME_PRIVATE_API ten_extensionhdr_t *ten_extensionhdr_create_for_extension(
    ten_extension_t *extension);

TEN_RUNTIME_PRIVATE_API ten_extensionhdr_t *
ten_extensionhdr_create_for_extension_info(ten_weak_ptr_t *extension_info);

TEN_RUNTIME_PRIVATE_API void ten_extensionhdr_destroy(ten_extensionhdr_t *self);
