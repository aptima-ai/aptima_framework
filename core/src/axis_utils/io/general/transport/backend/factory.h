//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include "axis_utils/io/general/transport/backend/base.h"

axis_UTILS_PRIVATE_API axis_transportbackend_factory_t *
axis_get_transportbackend_factory(const char *choise, const axis_string_t *uri);
