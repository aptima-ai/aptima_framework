//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_runtime/axis_config.h"

#include <stdbool.h>

#include "include_internal/axis_runtime/addon/addon.h"
#include "include_internal/axis_runtime/binding/python/common/python_stuff.h"
#include "axis_runtime/addon/addon.h"
#include "axis_utils/lib/signature.h"

#define axis_PY_ADDON_SIGNATURE 0xCFA1993E497A3D10U

typedef struct axis_py_addon_t {
  PyObject_HEAD

  axis_signature_t signature;
  axis_ADDON_TYPE type;

  // Note that this field is _not_ a pointer, but an actual axis_addon_t entity,
  // used as the addon entity registered to the aptima runtime.
  axis_addon_t c_addon;

  axis_addon_host_t *c_addon_host;
} axis_py_addon_t;

axis_RUNTIME_PRIVATE_API bool axis_py_addon_init_for_module(PyObject *module);

axis_RUNTIME_PRIVATE_API PyTypeObject *axis_py_addon_py_type(void);
