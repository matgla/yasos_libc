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

#include <langinfo.h>

#include <stddef.h>

#include <stdio.h>

/* The C locale's answer for each item (POSIX, XBD 7.3); "" for the items the
 * C locale leaves empty and for anything unknown. */
static const char *const c_locale_items[] = {
    [CODESET] = "UTF-8",
    [D_T_FMT] = "%a %b %e %H:%M:%S %Y",
    [D_FMT] = "%m/%d/%y",
    [T_FMT] = "%H:%M:%S",
    [T_FMT_AMPM] = "%I:%M:%S %p",
    [AM_STR] = "AM",
    [PM_STR] = "PM",
    [DAY_1] = "Sunday", [DAY_2] = "Monday", [DAY_3] = "Tuesday", [DAY_4] = "Wednesday",
    [DAY_5] = "Thursday", [DAY_6] = "Friday", [DAY_7] = "Saturday",
    [ABDAY_1] = "Sun", [ABDAY_2] = "Mon", [ABDAY_3] = "Tue", [ABDAY_4] = "Wed",
    [ABDAY_5] = "Thu", [ABDAY_6] = "Fri", [ABDAY_7] = "Sat",
    [MON_1] = "January", [MON_2] = "February", [MON_3] = "March", [MON_4] = "April",
    [MON_5] = "May", [MON_6] = "June", [MON_7] = "July", [MON_8] = "August",
    [MON_9] = "September", [MON_10] = "October", [MON_11] = "November", [MON_12] = "December",
    [ABMON_1] = "Jan", [ABMON_2] = "Feb", [ABMON_3] = "Mar", [ABMON_4] = "Apr",
    [ABMON_5] = "May", [ABMON_6] = "Jun", [ABMON_7] = "Jul", [ABMON_8] = "Aug",
    [ABMON_9] = "Sep", [ABMON_10] = "Oct", [ABMON_11] = "Nov", [ABMON_12] = "Dec",
    [RADIXCHAR] = ".",
    [THOUSEP] = "",
    [YESEXPR] = "^[yY]",
    [NOEXPR] = "^[nN]",
    [CRNCYSTR] = "-",
};

char *nl_langinfo(nl_item item) {
  if (item < 0 || item >= (nl_item)(sizeof(c_locale_items) / sizeof(c_locale_items[0])) ||
      !c_locale_items[item])
    return (char *)"";
  return (char *)c_locale_items[item];
}

char *nl_langinfo_l(nl_item item, locale_t locale) {
  (void)locale;
  return nl_langinfo(item);
}
