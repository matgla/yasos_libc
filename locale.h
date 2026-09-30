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

typedef unsigned char locale_t;

char *setlocale(int category, const char *locale);
locale_t uselocale(locale_t locale);
locale_t newlocale(int category_mask, const char *locale, locale_t base);
void freelocale(locale_t locobj);

#define LC_CTYPE 1
#define LC_CTYPE_MASK (1 << LC_CTYPE)
#define LC_NUMERIC 0
#define LC_NUMERIC_MASK (1 << LC_NUMERIC)
#define LC_TIME 2
#define LC_TIME_MASK (1 << LC_TIME)
#define LC_COLLATE 3
#define LC_COLLATE_MASK (1 << LC_COLLATE)
#define LC_MONETARY 4
#define LC_MONETARY_MASK (1 << LC_MONETARY)
#define LC_MESSAGES 5
#define LC_MESSAGES_MASK (1 << LC_MESSAGES)

#define LC_ALL 6