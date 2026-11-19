/* strptime against the host's glibc: the same input, format and starting
 * struct tm must give the same fields and stop at the same character. */

#define _GNU_SOURCE /* host strptime, tm_gmtoff */
#include "utest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *sut_strptime(const char *s, const char *fmt, struct tm *tm);

static const char *const cases[][2] = {
    /* toybox xparsedate's formats (date -d, touch -d) */
    {"2025-07-01 12:34:56", "%Y-%m-%d %T"},
    {"2025-07-01T12:34:56", "%Y-%m-%dT%T"},
    {"Tue Jul  1 12:34:56 CEST 2025", "%a %b %e %H:%M:%S %Z %Y"},
    {"Thu Nov 19 11:52:55 UTC 2026", "%a %b %e %H:%M:%S %Z %Y"},
    {"12:34:56", "%H:%M:%S"},
    {"2025-07-01 12:34", "%Y-%m-%d %H:%M"},
    {"2025-07-01", "%Y-%m-%d"},
    {"12:34", "%H:%M"},
    {"07011234", "%m%d%H%M"},
    {"0701123425", "%m%d%H%M%y"},
    {"2507011234", "%y%m%d%H%M"},
    {"070112342025", "%m%d%H%M%C%y"},
    {"202507011234", "%C%y%m%d%H%M"},
    {"2025-07-01 12:34:56.789", "%Y-%m-%d %T"}, /* stops at the fraction */
    {"2025-07-01 12:34:56 +0200", "%Y-%m-%d %T"},
    /* sort -M */
    {"Mar", "%b"},
    {"march 3", "%B %d"},
    {"DEC", "%b"},
    /* names */
    {"Wednesday", "%A"},
    {"wed", "%a"},
    {"Sept", "%b"}, /* "Sep" matches, "t" is left */
    /* 12-hour clock */
    {"12:00:00 AM", "%r"},
    {"12:30:00 PM", "%r"},
    {"01:05 pm", "%I:%M %p"},
    {"PM 11", "%p %I"},
    /* centuries and two-digit years */
    {"68", "%y"},
    {"69", "%y"},
    {"20", "%C"},
    {"19 99", "%C %y"},
    {"07/04/76", "%D"},
    {"07/04/76", "%x"},
    {"2024-366", "%Y-%j"},
    {"2025-060", "%Y-%j"},
    {"2024 60", "%Y %j"},
    /* white space and padding */
    {"  2025 -  7 -  1", "%Y - %m - %d"},
    {"2025\t07\n01", "%Y %m %d"},
    {" 7", "%e"},
    {"5", "%d"},
    /* others */
    {"2025-07-01", "%F"},
    {"12:34:56", "%T"},
    {"12:34:56", "%X"},
    {"Tue Jul  1 12:34:56 2025", "%c"},
    {"10%", "%y%%"},
    {"3", "%u"},
    {"7", "%u"},
    {"0", "%w"},
    {"23 2025", "%U %Y"},
    {"1751371200", "%s"},
    {"+0530", "%z"},
    {"-02:30", "%z"},
    {"Z", "%z"},
    {"+05", "%z"},
    {" 9", "%k"},
    {" 9", "%l"},
    {"2025", "%EY"},
    {"07", "%Om"},
    /* failures */
    {"2025-13-01", "%Y-%m-%d"},
    {"2025-02-32", "%Y-%m-%d"},
    {"24:00", "%H:%M"},
    {"00:00", "%I:%M"},
    {"xyz", "%a"},
    {"2025/07/01", "%Y-%m-%d"},
    {"", "%Y"},
    {"12", "%H:%M"},
    {"QM", "%p"},
    {"2025", "%Q"},
};

static void base_tm(struct tm *tm) {
  memset(tm, 0, sizeof(*tm));
  tm->tm_year = 125;
  tm->tm_mon = 10;
  tm->tm_mday = 19;
  tm->tm_hour = 7;
  tm->tm_min = 8;
  tm->tm_sec = 9;
  tm->tm_wday = 3;
  tm->tm_yday = 322;
  tm->tm_isdst = -1;
}

UTEST(strptime, matches_glibc) {
  int parsed = 0;
  setenv("TZ", "UTC0", 1);
  tzset();
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    const char *in = cases[i][0], *fmt = cases[i][1];
    struct tm want, got;
    base_tm(&want);
    base_tm(&got);
    const char *w = strptime(in, fmt, &want);
    const char *g = sut_strptime(in, fmt, &got);
    int same = (w == NULL) == (g == NULL);
    parsed += g != NULL;
    if (same && w) {
      same = w == g && want.tm_sec == got.tm_sec && want.tm_min == got.tm_min &&
             want.tm_hour == got.tm_hour && want.tm_mday == got.tm_mday &&
             want.tm_mon == got.tm_mon && want.tm_year == got.tm_year &&
             want.tm_wday == got.tm_wday && want.tm_yday == got.tm_yday &&
             want.tm_gmtoff == got.tm_gmtoff;
    }
    if (!same) {
      fprintf(stderr, "strptime(\"%s\", \"%s\"):\n", in, fmt);
      fprintf(stderr, "  glibc: %s rest \"%s\" %d-%d-%d %d:%d:%d wday %d yday %d gmtoff %ld\n",
              w ? "ok" : "NULL", w ? w : "", want.tm_year, want.tm_mon, want.tm_mday,
              want.tm_hour, want.tm_min, want.tm_sec, want.tm_wday, want.tm_yday, want.tm_gmtoff);
      fprintf(stderr, "  sut  : %s rest \"%s\" %d-%d-%d %d:%d:%d wday %d yday %d gmtoff %ld\n",
              g ? "ok" : "NULL", g ? g : "", got.tm_year, got.tm_mon, got.tm_mday, got.tm_hour,
              got.tm_min, got.tm_sec, got.tm_wday, got.tm_yday, got.tm_gmtoff);
    }
    EXPECT_TRUE(same);
  }
  /* All but the ten failure cases at the end parse. */
  EXPECT_EQ((int)(sizeof(cases) / sizeof(cases[0])) - 10, parsed);
}
