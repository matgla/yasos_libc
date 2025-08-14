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

#include <sys/socket.h>

#include <stdio.h>

int socket(int domain, int type, int protocol) {
  printf("TODO: Implement socket\n");
  return -1;
}

int setsockopt(int socket, int level, int option_name, const void *option_value,
               socklen_t option_len) {
  printf("TODO: Implement setsockopt\n");
  return -1;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  printf("TODO: Implement bind\n");
  return -1;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  printf("TODO: Implement connect\n");
  return -1;
}

int shutdown(int sockfd, int how) {
  printf("TODO: Implement shutdown\n");
  return -1;
}

ssize_t sendto(int socket, const void *message, size_t length, int flags,
               const struct sockaddr *dest_addr, socklen_t dest_len) {
  printf("TODO: Implement sendto\n");
  return -1;
}

ssize_t recvfrom(int socket, void *buffer, size_t length, int flags,
                 struct sockaddr *address, socklen_t *address_len) {
  printf("TODO: Implement recvfrom\n");
  return -1;
}