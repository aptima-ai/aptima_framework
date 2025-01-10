//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_utils/lib/error.h"
#include "ten_utils/lib/smart_ptr.h"

typedef struct ten_app_t ten_app_t;
typedef struct ten_connection_t ten_connection_t;
typedef struct ten_engine_t ten_engine_t;

TEN_RUNTIME_PRIVATE_API void ten_app_push_to_in_msgs_queue(
    ten_app_t *self, ten_shared_ptr_t *msg);

TEN_RUNTIME_PRIVATE_API bool ten_app_handle_out_msg(ten_app_t *self,
                                                    ten_shared_ptr_t *msg,
                                                    ten_error_t *err);

TEN_RUNTIME_PRIVATE_API bool ten_app_handle_in_msg(ten_app_t *self,
                                                   ten_connection_t *connection,
                                                   ten_shared_ptr_t *msg,
                                                   ten_error_t *err);

TEN_RUNTIME_PRIVATE_API ten_connection_t *ten_app_find_src_connection_for_msg(
    ten_app_t *self, ten_shared_ptr_t *msg);

TEN_RUNTIME_PRIVATE_API void
ten_app_do_connection_migration_or_push_to_engine_queue(
    ten_connection_t *connection, ten_engine_t *engine, ten_shared_ptr_t *msg);
