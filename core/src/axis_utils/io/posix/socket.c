//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "axis_utils/lib/alloc.h"
#include "axis_utils/log/log.h"
#include "axis_utils/macro/check.h"
#include "axis_utils/macro/mark.h"

axis_socket_t *axis_socket_create(axis_SOCKET_FAMILY family, axis_SOCKET_TYPE type,
                                axis_SOCKET_PROTOCOL protocol) {
  int32_t native_type = SOCK_STREAM;
  switch (type) {
    case axis_SOCKET_TYPE_STREAM:
      native_type = SOCK_STREAM;
      break;

    case axis_SOCKET_TYPE_DATAGRAM:
      native_type = SOCK_DGRAM;
      break;

    default:
      axis_LOGE("Unknown socket type: %d", type);
      return NULL;
  }

  int fd = socket(family, native_type, protocol);
  if (fd < 0) {
    axis_LOGE("Failed to create socket, fd: %d, errno: %d", fd, errno);
    return NULL;
  }

  axis_socket_t *ret = axis_MALLOC(sizeof(axis_socket_t));
  axis_ASSERT(ret, "Failed to allocate memory.");

  ret->fd = fd;
  ret->family = family;
  ret->protocol = protocol;
  ret->type = type;

  return ret;
}

void axis_socket_destroy(axis_socket_t *self) {
  axis_ASSERT(self, "Invalid argument.");

  axis_UNUSED int rc = close(self->fd);
  axis_ASSERT(!rc, "Failed to close socket: %d(errno: %d)", rc, errno);

  axis_FREE(self);
}
