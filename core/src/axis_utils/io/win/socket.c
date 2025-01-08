//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/socket.h"

#include <WS2tcpip.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ws2ipdef.h>

axis_socket_t *axis_socket_create(axis_SOCKET_FAMILY family, axis_SOCKET_TYPE type,
                                axis_SOCKET_PROTOCOL protocol) {
  int32_t native_type;
  switch (type) {
    case axis_SOCKET_TYPE_STREAM:
      native_type = SOCK_STREAM;
      break;

    case axis_SOCKET_TYPE_DATAGRAM:
      native_type = SOCK_DGRAM;
      break;

    default:
      // axis_LOGE("Unknown socket type.");
      return NULL;
  }

  // Initialize Winsock. This is a must before using any Winsock API.
  WSADATA wsaData;
  int rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (rc) {
    return NULL;
  }

  int fd = socket(family, native_type, protocol);
  if (fd == INVALID_SOCKET) {
    // axis_LOGE("Failed to create socket.");
    return NULL;
  }

  axis_socket_t *ret = malloc(sizeof(axis_socket_t));
  assert(ret);
  ret->fd = fd;
  ret->family = family;
  ret->protocol = protocol;
  ret->type = type;

  return ret;
}

void axis_socket_destroy(axis_socket_t *self) {
  assert(self);

  CloseHandle((HANDLE)self->fd);
  free(self);
}
