//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#pragma once

typedef enum TEN_MSG_FIELD {
  // 'type' should be the first, because the handling method of some fields
  // would depend on the 'type'.
  TEN_MSG_FIELD_TYPE,

  TEN_MSG_FIELD_NAME,

  TEN_MSG_FIELD_SRC,
  TEN_MSG_FIELD_DEST,
  TEN_MSG_FIELD_PROPERTIES,

  TEN_MSG_FIELD_LAST,
} TEN_MSG_FIELD;
