/*
 Copyright (c) 2026 Mateusz Stadnik

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma yasc
#include <libgen.h>

#include <string.h>

/* PATH is read up to its NUL, however long: POSIX sets no limit (no pathname
   resolution, "always successful", no errors defined).  A NUL missing from
   the caller's buffer is the compiler's check, at the call (libgen.h) */

/// @param s - the path slice (its trailing slashes are skipped here)
/// @returns the length of the parent directory, or 0 if there is none (".")
static size_t calculate_dirname_length([]const char s)
{
    size_t i;
    if (s.len == 0)
        return 0;
    i = s.len - 1;

    /* remove trailing slashes */
    while (i > 0 && s[i] == '/')
        i--;
    /* remove the last component */
    while (i > 0 && s[i] != '/')
        i--;
    if (s[i] != '/')
        return 0;
    /* slashes separating it from its parent */
    while (i > 0 && s[i] == '/')
        i--;
    return i + 1;
}

/* where s's last component starts, its trailing slashes cut off (one is
   kept when s is nothing else) */
static size_t basename_start([]char s)
{
    size_t i;
    if (s.len == 0)
        return 0;
    i = s.len - 1;
    for (; i > 0 && s[i] == '/'; i--)
        s[i] = '\0';
    /* back to the slash before the last component */
    for (; i > 0 && s[i - 1] != '/'; i--)
        ;
    return i;
}

char *borrows(static, path) dirname(inout char * maybe path)
{
    static char dot[] = ".";
    size_t len, end;
    if (path == nullptr || (len = strlen(path)) == 0)
        return dot;
    [:0]char s = path[0:len:0];
    end = calculate_dirname_length(s);
    if (end == 0)
        return dot;
    s[end] = '\0';
    return path;
}

char *borrows(static, path) basename(inout char * maybe path)
{
    static char dot[] = ".";
    size_t len;
    if (path == nullptr || (len = strlen(path)) == 0)
        return dot;
    return path + basename_start(path[0:len]);
}
