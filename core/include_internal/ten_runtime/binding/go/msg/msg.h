//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_runtime/ten_config.h"

#include "src/ten_runtime/binding/go/interface/ten/msg.h"

#define TEN_GO_MSG_SIGNATURE 0xB0E144BC5D3B1AB9U

TEN_RUNTIME_PRIVATE_API ten_go_msg_t *ten_go_msg_reinterpret(uintptr_t msg);

TEN_RUNTIME_PRIVATE_API bool ten_go_msg_check_integrity(ten_go_msg_t *self);

TEN_RUNTIME_PRIVATE_API ten_go_handle_t ten_go_msg_go_handle(ten_go_msg_t *self);

TEN_RUNTIME_PRIVATE_API ten_shared_ptr_t *ten_go_msg_c_msg(ten_go_msg_t *self);

TEN_RUNTIME_PRIVATE_API ten_shared_ptr_t *ten_go_msg_move_c_msg(
    ten_go_msg_t *self);

TEN_RUNTIME_PRIVATE_API ten_go_msg_t *ten_go_msg_create(ten_shared_ptr_t *c_msg);

TEN_RUNTIME_PRIVATE_API void ten_go_msg_set_go_handle(ten_go_msg_t *self,
                                                    ten_go_handle_t go_handle);
