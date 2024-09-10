//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/protocol/protocol.h"

typedef struct ten_protocol_context_t ten_protocol_context_t;
typedef struct ten_app_t ten_app_t;
typedef struct ten_protocol_context_store_t ten_protocol_context_store_t;

TEN_RUNTIME_API bool ten_protocol_context_store_check_integrity(
    ten_protocol_context_store_t *self, bool thread_check);

TEN_RUNTIME_API void ten_protocol_context_store_attach_to_app(
    ten_protocol_context_store_t *self, ten_app_t *app);

/**
 * @note If @a protocol_context is added into @a self, the reference count of @a
 * protocol_context will be increased by 1.
 */
TEN_RUNTIME_API bool ten_protocol_context_store_add_context_if_absent(
    ten_protocol_context_store_t *self,
    ten_protocol_context_t *protocol_context);

/**
 * @note If the protocol context is found, its reference count will be increased
 * by 1. Please keep in mind that you have to decrease the reference count of
 * the protocol context if you do not use it anymore.
 */
TEN_RUNTIME_API ten_protocol_context_t *
ten_protocol_context_store_find_first_context_with_role(
    ten_protocol_context_store_t *self, const char *protocol_name,
    TEN_PROTOCOL_ROLE role);

TEN_RUNTIME_API ten_runloop_t *ten_protocol_context_store_get_attached_runloop(
    ten_protocol_context_store_t *self);