//
// This file is part of the TEN Framework project.
// See https://github.com/TEN-framework/ten_framework/LICENSE for license
// information.
//
#include "ten_utils/io/socket.h"

#include <WS2tcpip.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ws2ipdef.h>

ten_socket_t *ten_socket_create(TEN_SOCKET_FAMILY family, TEN_SOCKET_TYPE type,
                                TEN_SOCKET_PROTOCOL protocol) {
  int32_t native_type;
  switch (type) {
    case TEN_SOCKET_TYPE_STREAM:
      native_type = SOCK_STREAM;
      break;

    case TEN_SOCKET_TYPE_DATAGRAM:
      native_type = SOCK_DGRAM;
      break;

    default:
      // TEN_LOGE("Unknown socket type.");
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
    // TEN_LOGE("Failed to create socket.");
    return NULL;
  }

  ten_socket_t *ret = malloc(sizeof(ten_socket_t));
  assert(ret);
  ret->fd = fd;
  ret->family = family;
  ret->protocol = protocol;
  ret->type = type;

  return ret;
}

void ten_socket_destroy(ten_socket_t *self) {
  assert(self);

  CloseHandle((HANDLE)self->fd);
  free(self);
}
