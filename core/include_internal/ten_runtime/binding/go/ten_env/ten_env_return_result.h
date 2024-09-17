//
// Copyright © 2024 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "ten_runtime/ten_config.h"

#include "ten_runtime/binding/go/interface/ten/common.h"
#include "ten_runtime/binding/go/interface/ten/ten_env.h"
#include "ten_runtime/common/status_code.h"
#include "ten_utils/value/value.h"

TEN_RUNTIME_PRIVATE_API bool ten_go_ten_return_status_value(
    ten_go_ten_env_t *self, ten_go_msg_t *cmd, TEN_STATUS_CODE status_code,
    ten_value_t *status_value, ten_go_status_t *api_status);
