//
// Copyright © 2025 Agora
// This file is part of APTIMA Framework, an open source project.
// Licensed under the Apache License, Version 2.0, with certain conditions.
// Refer to the "LICENSE" file in the root directory for more information.
//
#include "axis_utils/io/network.h"

#include <arpa/inet.h>
#include <assert.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(axis_ENABLE_OWN_IFADDR)
axis_UTILS_API int getifaddrs(struct ifaddrs **ifap);
axis_UTILS_API void freeifaddrs(struct ifaddrs *ifa);

void axis_host_get(char *hostname_buffer,
                  axis_UNUSED const size_t hostname_buffer_capacity,
                  char *ip_buffer, const size_t ip_buffer_capacity) {
  assert(hostname_buffer && hostname_buffer_capacity == URI_MAX_LEN &&
         ip_buffer && ip_buffer_capacity == IP_STR_MAX_LEN);

  // To retrieve hostname
  axis_UNUSED int rc = gethostname(hostname_buffer, hostname_buffer_capacity);
  assert(rc != -1);

  // axis_LOGD("Hostname: %s", hostname_buffer);

  // To retrieve host information, and get IP from it.
  struct ifaddrs *myaddrs = NULL;
  rc = getifaddrs(&myaddrs);
  assert(rc == 0);

  for (struct ifaddrs *ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next) {
    if ((ifa->ifa_addr == NULL) ||
        ((ifa->ifa_flags & (unsigned int)IFF_UP) == 0)) {
      continue;
    }

    void *in_addr = NULL;
    switch (ifa->ifa_addr->sa_family) {
      case AF_INET: {
        struct sockaddr_in *s4 = (struct sockaddr_in *)(ifa->ifa_addr);
        in_addr = &s4->sin_addr;
        break;
      }
      case AF_INET6: {
        struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);
        in_addr = &s6->sin6_addr;
        break;
      }
      default:
        continue;
    }

    axis_UNUSED const char *result = inet_ntop(ifa->ifa_addr->sa_family, in_addr,
                                              ip_buffer, ip_buffer_capacity);
    assert(result != NULL);

    // axis_LOGD("IP: %s", ip_buffer);
    break;
  }

  freeifaddrs(myaddrs);
}
#endif

bool axis_get_ipv6_prefix(const char *ifid, axis_string_t *prefix) {
  return false;
}
