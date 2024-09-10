//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

#include "ten_utils/ten_config.h"

#include "ten_utils/io/transport.h"

TEN_UTILS_PRIVATE_API enum TEN_TRANSPORT_DROP_TYPE ten_transport_get_drop_type(
    ten_transport_t *self);

TEN_UTILS_PRIVATE_API void ten_transport_set_drop_type(
    ten_transport_t *self, TEN_TRANSPORT_DROP_TYPE drop_type);

TEN_UTILS_PRIVATE_API int ten_transport_drop_required(ten_transport_t *self);

TEN_UTILS_PRIVATE_API void ten_transport_set_drop_when_full(
    ten_transport_t *self, int drop);

TEN_UTILS_PRIVATE_API void ten_transport_on_close(ten_transport_t *self);
