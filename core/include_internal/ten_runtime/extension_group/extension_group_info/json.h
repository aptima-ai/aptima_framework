//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/container/list.h"
#include "ten_utils/lib/error.h"
#include "ten_utils/lib/json.h"
#include "ten_utils/lib/smart_ptr.h"

typedef struct ten_extension_group_info_t ten_extension_group_info_t;

TEN_RUNTIME_PRIVATE_API ten_json_t *ten_extension_group_info_to_json(
    ten_extension_group_info_t *self);

TEN_RUNTIME_PRIVATE_API ten_shared_ptr_t *ten_extension_group_info_from_json(
    ten_json_t *json, ten_list_t *extension_groups_info, ten_error_t *err);
