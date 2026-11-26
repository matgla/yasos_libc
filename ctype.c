// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

/* The out-of-line definitions; <ctype.h> would otherwise map these names to
 * its inline versions. */
#define __YASLIBC_CTYPE_IMPL
#include <ctype.h>

int isascii(int c) {
  return cast<trunc(unsigned)>(c) < 128u;
}

int isblank(int c) {
  return c == ' ' || c == '\t';
}

int isalpha(int c) {
  return cast<trunc(unsigned)>((c | 0x20) - 'a') < 26u;
}

int isdigit(int c) {
  return cast<trunc(unsigned)>(c - '0') < 10u;
}

int isalnum(int c) {
  return cast<trunc(unsigned)>((c | 0x20) - 'a') < 26u || cast<trunc(unsigned)>(c - '0') < 10u;
}

int isspace(int c) {
  return cast<trunc(unsigned)>(c - 9) < 5u || c == ' ';
}

int isupper(int c) {
  return cast<trunc(unsigned)>(c - 'A') < 26u;
}

int islower(int c) {
  return cast<trunc(unsigned)>(c - 'a') < 26u;
}

int tolower(int c) {
  return cast<trunc(unsigned)>(c - 'A') < 26u ? c + ('a' - 'A') : c;
}

int toupper(int c) {
  return cast<trunc(unsigned)>(c - 'a') < 26u ? c + ('A' - 'a') : c;
}

int isprint(int c) {
  return cast<trunc(unsigned)>(c - 32) < 95u;
}

int isgraph(int c) {
  return cast<trunc(unsigned)>(c - 33) < 94u;
}

int ispunct(int c) {
  return isprint(c) && !isalnum(c) && !isspace(c);
}

int isxdigit(int c) {
  return cast<trunc(unsigned)>((c | 0x20) - 'a') < 6u || cast<trunc(unsigned)>(c - '0') < 10u;
}

int iscntrl(int c) {
  return cast<trunc(unsigned)>(c) < 32u || c == 127;
}