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

#include <arpa/inet.h>

#include <stddef.h>

#include <stdio.h>

unsigned int htonl(unsigned int n) {
  printf("TODO: Implement htonl\n");
  return 0;
}

unsigned int ntohl(unsigned int n) {
  printf("TODO: Implement ntohl\n");
  return 0;
}

unsigned short htons(unsigned short n) {
  printf("TODO: Implement htons\n");
  return 0;
}

unsigned short ntohs(unsigned short n) {
  printf("TODO: Implement ntohs\n");
  return 0;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size) {
  printf("TODO: Implement inet_ntop\n");
  return NULL; // Return NULL to indicate failure, as the function is not
               // implemented.
}