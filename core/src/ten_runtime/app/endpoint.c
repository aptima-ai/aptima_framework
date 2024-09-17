//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "include_internal/ten_runtime/app/endpoint.h"

#include "include_internal/ten_runtime/app/app.h"
#include "include_internal/ten_runtime/app/close.h"
#include "include_internal/ten_runtime/connection/connection.h"
#include "include_internal/ten_runtime/protocol/close.h"
#include "include_internal/ten_runtime/protocol/context_store.h"
#include "include_internal/ten_runtime/protocol/protocol.h"
#include "include_internal/ten_utils/log/log.h"
#include "ten_utils/macro/check.h"

static ten_connection_t *create_connection_when_client_accepted(
    ten_protocol_t *protocol) {
  TEN_ASSERT(protocol && ten_protocol_check_integrity(protocol, true),
             "Should not happen.");
  TEN_ASSERT(ten_protocol_attach_to(protocol) == TEN_PROTOCOL_ATTACH_TO_APP,
             "Should not happen.");

  ten_app_t *app = protocol->attached_target.app;
  TEN_ASSERT(app && ten_app_check_integrity(app, true), "Should not happen.");

  TEN_LOGD("[%s] A client is connected.",
           ten_string_get_raw_str(ten_app_get_uri(app)));

  // Create a connection to represent it.
  ten_connection_t *connection = ten_connection_create(protocol);
  TEN_ASSERT(connection, "Should not happen.");

  ten_connection_attach_to_app(connection, app);

  return connection;
}

bool ten_app_create_endpoint(ten_app_t *self, ten_string_t *uri) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true), "Should not happen.");

  self->endpoint_protocol = ten_protocol_create(ten_string_get_raw_str(uri),
                                                TEN_PROTOCOL_ROLE_LISTEN);
  if (!self->endpoint_protocol) {
    return false;
  }

  TEN_ASSERT(self->endpoint_protocol, "Should not happen.");

  ten_protocol_attach_to_app(self->endpoint_protocol, self);
  ten_protocol_set_on_closed(self->endpoint_protocol,
                             ten_app_on_protocol_closed, self);

  ten_protocol_listen(self->endpoint_protocol, ten_string_get_raw_str(uri),
                      create_connection_when_client_accepted);

  return true;
}

bool ten_app_is_endpoint_closed(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true),
             "Access across threads.");

  if (!self->endpoint_protocol) {
    return true;
  }

  return ten_protocol_is_closed(self->endpoint_protocol);
}

void ten_app_create_protocol_context_store(ten_app_t *self) {
  TEN_ASSERT(self && ten_app_check_integrity(self, true),
             "Access across threads.");

  self->protocol_context_store = ten_protocol_context_store_create(
      offsetof(ten_protocol_context_store_item_t, hh_in_context_store));
  TEN_ASSERT(self->protocol_context_store,
             "Failed to create protocol context store.");

  ten_protocol_context_store_attach_to_app(self->protocol_context_store, self);

  ten_protocol_context_store_set_on_closed(
      self->protocol_context_store, ten_app_on_protocol_context_store_closed,
      self);
}
