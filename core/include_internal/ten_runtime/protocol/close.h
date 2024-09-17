//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include <stdbool.h>

#include "include_internal/ten_runtime/common/closeable.h"

typedef struct ten_protocol_t ten_protocol_t;

typedef void (*ten_protocol_on_closed_func_t)(ten_protocol_t *self,
                                              void *on_closed_data);

TEN_RUNTIME_PRIVATE_API bool ten_protocol_is_closing(ten_protocol_t *self);

TEN_RUNTIME_PRIVATE_API bool ten_protocol_is_closed(ten_protocol_t *self);

TEN_RUNTIME_PRIVATE_API void ten_protocol_set_on_closed(
    ten_protocol_t *self, ten_protocol_on_closed_func_t on_closed,
    void *on_closed_data);

TEN_RUNTIME_PRIVATE_API void ten_protocol_on_close(ten_protocol_t *self);

TEN_RUNTIME_API void ten_protocol_action_to_close_myself(
    ten_closeable_t *self_, void *close_myself_data,
    ten_closeable_action_to_close_myself_done_func_t close_myself_done);

TEN_RUNTIME_API void ten_protocol_on_impl_intends_to_close(
    ten_closeable_t *impl, void *self_closeable, void *on_intend_to_close_data);

TEN_RUNTIME_API void ten_protocol_on_impl_closed_all_done(
    ten_closeable_t *impl, void *self_closeable, void *on_closed_all_done_data);
