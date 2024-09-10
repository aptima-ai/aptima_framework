//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "ten_runtime/protocol/protocol.h"

typedef struct ten_protocol_context_store_t ten_protocol_context_store_t;
typedef struct ten_protocol_context_t ten_protocol_context_t;

// When the protocol context receives a close signal, try to close the
// implementation first.
typedef void (*ten_protocol_context_close_impl_func_t)(void *impl);

// When all the closing flow of the protocol layer is completed, the destroying
// flow of the protocol layer could be started. The implementation protocol
// context should be destroyed before the TEN protocol context.
typedef void (*ten_protocol_context_destroy_impl_func_t)(void *implementation);

TEN_RUNTIME_API ten_protocol_context_t *ten_protocol_context_create(
    ten_protocol_context_store_t *context_store, const char *protocol_name,
    ten_protocol_context_close_impl_func_t close_impl,
    ten_protocol_context_destroy_impl_func_t destroy_impl,
    void *impl_protocol_context);

TEN_RUNTIME_API ten_protocol_context_t *ten_protocol_context_create_with_role(
    ten_protocol_context_store_t *context_store, const char *protocol_name,
    TEN_PROTOCOL_ROLE role, ten_protocol_context_close_impl_func_t close_impl,
    ten_protocol_context_destroy_impl_func_t destroy_impl,
    void *impl_protocol_context);

TEN_RUNTIME_API bool ten_protocol_context_is_closing(
    ten_protocol_context_t *self);

TEN_RUNTIME_API void ten_protocol_context_on_implemented_closed_async(
    ten_protocol_context_t *self);
