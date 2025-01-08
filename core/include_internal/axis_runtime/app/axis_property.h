//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include "include_internal/axis_runtime/app/metadata.h"
#include "include_internal/axis_runtime/common/constant_str.h"

static const axis_app_axis_namespace_prop_info_t
    axis_app_axis_namespace_prop_info_list[] = {
        {
            .name = axis_STR_ONE_EVENT_LOOP_PER_ENGINE,
            .init_from_value = axis_app_init_one_event_loop_per_engine,
        },
        {
            .name = axis_STR_LONG_RUNNING_MODE,
            .init_from_value = axis_app_init_long_running_mode,
        },
        {
            .name = axis_STR_URI,
            .init_from_value = axis_app_init_uri,
        },
        {
            .name = axis_STR_LOG_LEVEL,
            .init_from_value = axis_app_init_log_level,
        },
        {
            .name = axis_STR_LOG_FILE,
            .init_from_value = axis_app_init_log_file,
        }};

static const size_t axis_app_axis_namespace_prop_info_list_size =
    sizeof(axis_app_axis_namespace_prop_info_list) /
    sizeof(axis_app_axis_namespace_prop_info_list[0]);
