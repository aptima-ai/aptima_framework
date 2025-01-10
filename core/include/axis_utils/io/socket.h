//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#pragma once

#include "aptima_utils/aptima_config.h"

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

#include "aptima_utils/lib/string.h"

typedef enum aptima_SOCKET_FAMILY {
  aptima_SOCKET_FAMILY_INET = AF_INET,   // IPv4
  aptima_SOCKET_FAMILY_INET6 = AF_INET6  // IPv6
} aptima_SOCKET_FAMILY;

typedef enum aptima_SOCKET_TYPE {
  aptima_SOCKET_TYPE_STREAM = 1,    // TCP
  aptima_SOCKET_TYPE_DATAGRAM = 2,  // UDP
} aptima_SOCKET_TYPE;

typedef enum aptima_SOCKET_PROTOCOL {
  aptima_SOCKET_PROTOCOL_TCP = 6,
  aptima_SOCKET_PROTOCOL_UDP = 17,
} aptima_SOCKET_PROTOCOL;

typedef struct aptima_addr_port_t {
  aptima_string_t *addr;
  uint16_t port;
} aptima_addr_port_t;

typedef struct aptima_socket_addr_t {
  aptima_SOCKET_FAMILY family;
  union {
    struct in_addr sin_addr;
    struct in6_addr sin6_addr;
  } addr;
  uint16_t port;
} aptima_socket_addr_t;

typedef struct aptima_socket_t {
  aptima_SOCKET_FAMILY family;
  aptima_SOCKET_PROTOCOL protocol;
  aptima_SOCKET_TYPE type;
  int fd;
} aptima_socket_t;

// Socket address
aptima_UTILS_API aptima_socket_addr_t *aptima_socket_addr_create(const char *address,
                                                        uint16_t port);

aptima_UTILS_API void aptima_socket_addr_destroy(aptima_socket_addr_t *self);

// Socket
aptima_UTILS_API aptima_socket_t *aptima_socket_create(aptima_SOCKET_FAMILY family,
                                              aptima_SOCKET_TYPE type,
                                              aptima_SOCKET_PROTOCOL protocol);

aptima_UTILS_API void aptima_socket_destroy(aptima_socket_t *self);

aptima_UTILS_API bool aptima_socket_connect(aptima_socket_t *socket,
                                      aptima_socket_addr_t *address);

aptima_UTILS_API ssize_t aptima_socket_send(const aptima_socket_t *self, void *buf,
                                      size_t buf_size);

aptima_UTILS_API ssize_t aptima_socket_recv(const aptima_socket_t *socket, void *buf,
                                      size_t buf_size);

aptima_UTILS_API aptima_addr_port_t
aptima_socket_peer_addr_port(const aptima_socket_t *self);

aptima_UTILS_API void aptima_socket_get_info(aptima_socket_t *self, aptima_string_t *ip,
                                       uint16_t *port);
