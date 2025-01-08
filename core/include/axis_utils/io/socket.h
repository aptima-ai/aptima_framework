//
// Copyright © 2025 Agora
// This file is part of TEN Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "axis_utils/axis_config.h"

#include <stdint.h>
#if defined(_WIN32)
// clang-format off
#include <Windows.h>
#include <In6addr.h>
#include <WinSock2.h>
// clang-format on
#else
#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include "axis_utils/lib/string.h"

typedef enum axis_SOCKET_FAMILY {
  axis_SOCKET_FAMILY_INET = AF_INET,   // IPv4
  axis_SOCKET_FAMILY_INET6 = AF_INET6  // IPv6
} axis_SOCKET_FAMILY;

typedef enum axis_SOCKET_TYPE {
  axis_SOCKET_TYPE_STREAM = 1,    // TCP
  axis_SOCKET_TYPE_DATAGRAM = 2,  // UDP
} axis_SOCKET_TYPE;

typedef enum axis_SOCKET_PROTOCOL {
  axis_SOCKET_PROTOCOL_TCP = 6,
  axis_SOCKET_PROTOCOL_UDP = 17,
} axis_SOCKET_PROTOCOL;

typedef struct axis_addr_port_t {
  axis_string_t *addr;
  uint16_t port;
} axis_addr_port_t;

typedef struct axis_socket_addr_t {
  axis_SOCKET_FAMILY family;
  union {
    struct in_addr sin_addr;
    struct in6_addr sin6_addr;
  } addr;
  uint16_t port;
} axis_socket_addr_t;

typedef struct axis_socket_t {
  axis_SOCKET_FAMILY family;
  axis_SOCKET_PROTOCOL protocol;
  axis_SOCKET_TYPE type;
  int fd;
} axis_socket_t;

// Socket address
axis_UTILS_API axis_socket_addr_t *axis_socket_addr_create(const char *address,
                                                        uint16_t port);

axis_UTILS_API void axis_socket_addr_destroy(axis_socket_addr_t *self);

// Socket
axis_UTILS_API axis_socket_t *axis_socket_create(axis_SOCKET_FAMILY family,
                                              axis_SOCKET_TYPE type,
                                              axis_SOCKET_PROTOCOL protocol);

axis_UTILS_API void axis_socket_destroy(axis_socket_t *self);

axis_UTILS_API bool axis_socket_connect(axis_socket_t *socket,
                                      axis_socket_addr_t *address);

axis_UTILS_API ssize_t axis_socket_send(const axis_socket_t *self, void *buf,
                                      size_t buf_size);

axis_UTILS_API ssize_t axis_socket_recv(const axis_socket_t *socket, void *buf,
                                      size_t buf_size);

axis_UTILS_API axis_addr_port_t
axis_socket_peer_addr_port(const axis_socket_t *self);

axis_UTILS_API void axis_socket_get_info(axis_socket_t *self, axis_string_t *ip,
                                       uint16_t *port);
