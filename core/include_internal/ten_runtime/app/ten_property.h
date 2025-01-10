//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "include_internal/ten_runtime/app/metadata.h"
#include "include_internal/ten_runtime/common/constant_str.h"

static const ten_app_ten_namespace_prop_info_t
    ten_app_ten_namespace_prop_info_list[] = {
        {
            .name = TEN_STR_ONE_EVENT_LOOP_PER_ENGINE,
            .init_from_value = ten_app_init_one_event_loop_per_engine,
        },
        {
            .name = TEN_STR_LONG_RUNNING_MODE,
            .init_from_value = ten_app_init_long_running_mode,
        },
        {
            .name = TEN_STR_URI,
            .init_from_value = ten_app_init_uri,
        },
        {
            .name = TEN_STR_LOG_LEVEL,
            .init_from_value = ten_app_init_log_level,
        },
        {
            .name = TEN_STR_LOG_FILE,
            .init_from_value = ten_app_init_log_file,
        }};

static const size_t ten_app_ten_namespace_prop_info_list_size =
    sizeof(ten_app_ten_namespace_prop_info_list) /
    sizeof(ten_app_ten_namespace_prop_info_list[0]);
