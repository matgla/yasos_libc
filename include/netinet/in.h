#pragma once

#include <stdint.h>

typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;

typedef int sa_family_t;

struct in_addr {
  in_addr_t s_addr;
};

struct sockaddr_in {
  sa_family_t sin_family;  /* AF_INET */
  in_port_t sin_port;      /* Port number */
  struct in_addr sin_addr; /* IPv4 address */
};

struct in6_addr {
  uint8_t s6_addr[16];
};

struct sockaddr_in6 {
  sa_family_t sin6_family;   /* AF_INET6 */
  in_port_t sin6_port;       /* Port number */
  uint32_t sin6_flowinfo;    /* IPv6 flow info */
  struct in6_addr sin6_addr; /* IPv6 address */
  uint32_t sin6_scope_id;    /* Set of interfaces for a scope */
};
