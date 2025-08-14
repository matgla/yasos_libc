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

#include <stdarg.h>

void openlog(const char *ident, int option, int facility);
void syslog(int priority, const char *format, ...);
void closelog(void);
void vsyslog(int priority, const char *format, va_list ap);

#define LOG_CONS 0x01
#define LOG_NDELAY 0x02
#define LOG_NOWAIT 0x04
#define LOG_PERROR 0x08
#define LOG_PID 0x10

#define LOG_AUTH 0x0001
#define LOG_AUTHPRIV 0x0002
#define LOG_CRON 0x0004
#define LOG_DAEMON 0x0008
#define LOG_FTP 0x0010
#define LOG_KERN 0x0020
#define LOG_LOCAL0 0x0100
#define LOG_LOCAL1 0x0200
#define LOG_LOCAL2 0x0400
#define LOG_LOCAL3 0x0800
#define LOG_LOCAL4 0x1000
#define LOG_LOCAL5 0x2000
#define LOG_LOCAL6 0x4000
#define LOG_LOCAL7 0x8000
#define LOG_LPR 0x10000
#define LOG_MAIL 0x20000
#define LOG_NEWS 0x40000
#define LOG_SYSLOG 0x80000
#define LOG_USER 0x100000
#define LOG_UUCP 0x200000

#define LOG_EMERG 0x0001
#define LOG_ALERT 0x0002
#define LOG_CRIT 0x0004
#define LOG_ERR 0x0008
#define LOG_WARNING 0x0010
#define LOG_NOTICE 0x0020
#define LOG_INFO 0x0040
#define LOG_DEBUG 0x0080