//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "include_internal/axis_runtime/app/endpoint.h"

#include "include_internal/axis_runtime/app/app.h"
#include "include_internal/axis_runtime/app/close.h"
#include "include_internal/axis_runtime/connection/connection.h"
#include "include_internal/axis_runtime/protocol/close.h"
#include "include_internal/axis_runtime/protocol/protocol.h"
#include "axis_utils/lib/string.h"
#include "axis_utils/macro/check.h"

static axis_connection_t *create_connection_when_client_accepted(
    axis_protocol_t *listening_protocol, axis_protocol_t *protocol) {
  axis_ASSERT(listening_protocol &&
                 axis_protocol_check_integrity(listening_protocol, true),
             "Should not happen.");
  axis_ASSERT(protocol && axis_protocol_check_integrity(protocol, true),
             "Should not happen.");
  axis_ASSERT(axis_protocol_attach_to(protocol) == axis_PROTOCOL_ATTACH_TO_APP,
             "Should not happen.");

  axis_app_t *app = protocol->attached_target.app;
  axis_ASSERT(app && axis_app_check_integrity(app, true), "Should not happen.");

  axis_LOGD("[%s] A client is connected.", axis_app_get_uri(app));

  // Create a connection to represent it.
  axis_connection_t *connection = axis_connection_create(protocol);
  axis_ASSERT(connection, "Should not happen.");

  axis_connection_attach_to_app(connection, app);

  return connection;
}

bool axis_app_endpoint_listen(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true), "Should not happen.");

  if (!self->endpoint_protocol) {
    return false;
  }

  axis_protocol_listen(self->endpoint_protocol,
                      axis_string_get_raw_str(&self->uri),
                      create_connection_when_client_accepted);

  return true;
}

bool axis_app_is_endpoint_closed(axis_app_t *self) {
  axis_ASSERT(self && axis_app_check_integrity(self, true),
             "Access across threads.");

  if (!self->endpoint_protocol) {
    return true;
  }

  return axis_protocol_is_closed(self->endpoint_protocol);
}
