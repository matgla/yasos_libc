#pragma yasc
#include <sys/_attrs.h>
#pragma tcc c_begin
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <netinet/in.h>
#include <sys/types.h>

struct sockaddr {
  sa_family_t sa_family; /* Address family */
  char sa_data[];        /* Socket address */
};

struct sockaddr_storage {
  sa_family_t ss_family; /* Address family */
};

typedef unsigned int socklen_t;


#pragma tcc c_end
int owned(close) maybe socket(int domain, int type, int protocol);
int listen(int view sockfd, int backlog);
/* the connection's own descriptor; ADDR and ADDRLEN may be null together */
/* a new connection, which the listening socket does not own: closing one
   leaves the other open */
int owned(close) maybe accept(int view sockfd, out struct sockaddr * maybe addr, inout socklen_t * maybe addrlen);
int setsockopt(int view socket, int level, int option_name, const void *option_value,
               socklen_t option_len);
int bind(int view sockfd, const struct sockaddr *addr, socklen_t addrlen);
int connect(int view sockfd, const struct sockaddr *addr, socklen_t addrlen);
int shutdown(int view sockfd, int how);
/* DEST_ADDR may be null on a connected socket */
ssize_t sendto(int view socket, const void *message, size_t length, int flags,
               const struct sockaddr * maybe dest_addr, socklen_t dest_len);
/* ADDRESS and ADDRESS_LEN may be null together */
ssize_t recvfrom(int view socket, out void *buffer, size_t length, int flags,
                 out struct sockaddr * maybe address, inout socklen_t * maybe address_len);
#pragma tcc c_begin

#define SOL_SOCKET 1

#define SO_REUSEADDR 2

#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

#define AF_INET 2
#define AF_INET6 10
#pragma tcc c_end
