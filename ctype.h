// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>
#pragma once

int isascii(int c);
int isblank(int c);
int isspace(int c);
int isalpha(int c);
int isdigit(int c);
int isalnum(int c);
int isupper(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);

int tolower(int c);
int toupper(int c);

int iscntrl(int c);
int isgraph(int c);
int isxdigit(int c);
