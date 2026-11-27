/* yasc (-std=yasc -Wall): the same functions used correctly report nothing. */
#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <grp.h>
#include <locale.h>
#include <mntent.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdckdint.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <langinfo.h>
#include <netdb.h>
#include <regex.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <regex.h>

static int cmp(const void *a, const void *b) { return *cast<ptr(const int *view)>(a) - *cast<ptr(const int *view)>(b); }
int c_strchr(const char *s) { const char *p = strchr(s, 'x'); if (!p) return 0; return *p; }
int c_strstr(const char *s) { return strstr(s, "ab") ? 1 : 0; }
int c_memchr(const char *s) { const char *p = memchr(s, 'x', 4); return p ? *p : 0; }
int c_memchr_null(void) { return memchr(nullptr, 'x', 0) != nullptr; }
int c_strtok(inout char *s) { int n = 0; for (char *t = strtok(s, " "); t; t = strtok(nullptr, " ")) n += *t; return n; }
int c_strtok_r(inout char *s) { char *sp = nullptr; char *t = strtok_r(s, " ", &sp); return t ? *t : 0; }
int c_getenv(void) { char *e = getenv("HOME"); if (!e) return 0; return *e + cast<int(int)>(strlen(e)); }
int c_fgets(inout FILE *f) { char buf[8] = undefined; while (fgets(buf, 8, f)) if (buf[0] == 'x') return 1; return 0; }
int c_fopen(void) { char buf[8] = undefined; FILE *?owned f = fopen("x", "r"); if (!f) return -1; int r = fgets(buf, 8, f) != nullptr; fclose(f); return r; }
int c_readdir(void) { int n = 0; DIR *?owned d = opendir("/"); if (!d) return -1; struct dirent *e; while ((e = readdir(d))) n += e->d_type; closedir(d); return n; }
int c_localtime(void) { time_t t = time(nullptr); struct tm *tm = localtime(&t); return tm ? tm->tm_year : 0; }
int c_ctime_r(const time_t *t) { char b[26] = undefined; char *s = ctime_r(t, b); return s ? *s : 0; }
int c_getpw(void) { struct passwd *p = getpwuid(0); if (!p) return -1; return cast<as(int)>(p->pw_uid); }
int c_getgr(void) { struct group *g = getgrnam("wheel"); return g ? cast<as(int)>(g->gr_gid) : -1; }
int c_setlocale(void) { char *l = setlocale(0, nullptr); return l ? *l : 0; }
int c_dl(void) { void *s = dlsym(nullptr, "f"); const char *e = dlerror(); return (s != nullptr) + (e ? *e : 0); }
int c_mnt(void) { FILE *?owned f = setmntent("/etc/fstab", "r"); if (!f) return -1; int n = 0; struct mntent *m; while ((m = getmntent(f))) n += hasmntopt(m, "ro") != nullptr; endmntent(f); return n; }
int c_bsearch(int k) { static const int a[3] = {1, 2, 3}; const int *r = bsearch(&k, a, 3, sizeof a[0], cmp); return r ? *r : -1; }
int c_qsort(inout int *a) { qsort(a, 3, sizeof a[0], cmp); return a[0]; }
int c_tmpnam(void) { char *n = tmpnam(nullptr); return n ? *n : 0; }
int c_nulls(void) {
  /* null where it is allowed */
  char b[4] = undefined;
  fflush(nullptr); setvbuf(stdout, nullptr, _IONBF, 0); perror(nullptr);
  return snprintf(nullptr, 0, "%d", 1) + cast<int(int)>(strtol("1", nullptr, 10)) + cast<int(int)>(time(nullptr)) +
         (getcwd(nullptr, 0) != nullptr) + (realpath("/", nullptr) != nullptr) + setenv("A", nullptr, 1) +
         cast<int(int)>(sizeof b);
}
int c_paths(void) { struct stat st = undefined; int ?owned(close) fd = open("/x", O_RDONLY); if (fd >= 0) close(fd); return stat("/x", &st) + access("/x", 0) + unlink("/y"); }
void t_realloc(char *?owned p) { char *?owned q = realloc(move(p), 8); if (!q) { free(p); return; } free(q); }
const char *borrows(static) c_strerror(int e) { char *view s = strerror(e); return *s ? s : strerror(0); }
char *borrows(static) c_asctime(const struct tm *t) { return asctime(t); }
char *borrows(b) c_asctime_r(const struct tm *t, out char *b) { return asctime_r(t, b); }
char *borrows(static, p) c_dirname(inout char *?p) { return dirname(p); }
int c_basename(inout char *p) { char *b = basename(p); return *b + *dirname(nullptr) + *basename(nullptr); }
char *borrows(static) c_langinfo(void) { return nl_langinfo(CODESET); }
const char *borrows(static) c_gai(int e) { return gai_strerror(e); }
/* nothing bounds them (yasc_unsafe): inside unsafe they say nothing */
char *borrows(d) c_strcpy(out char *d, const char *s)
{
  unsafe {
    strcpy(d, s);
    strcat(d, s);
    return stpcpy(d, s);
  }
}
char *borrows(d) c_strlcpy(out char *d, const char *s) { strlcpy(d, s, 8); strlcat(d, s, 8); return d; }
char *borrows(s) c_itoa(out char *s) { return itoa(1, s, 10); }
const char *?borrows(d) c_inet_ntop(const void *s, out char *d) { return inet_ntop(AF_INET, s, d, 16); }
int c_getlogin(void) { char *l = getlogin(); return l ? *l : 0; }
int c_accept(void) { int ?owned(close) s = socket(AF_INET, 1, 0); if (s < 0) return -1; if (listen(s, 1)) { close(s); return -1; } int ?owned(close) c = accept(s, nullptr, nullptr); if (c >= 0) close(c); close(s); return 0; }
/* a plain int (pipe's, until owned out-parameters) and a constant */
int c_dup2(int fd) { return dup2(fd, 1) + dup2(0, 2); }
int c_dup2_handle(int ?owned(close) fd) { if (fd < 0) return -1; int r = dup2(fd, 1); close(fd); return r; }
int c_mkstemp(inout char *t) { int ?owned(close) fd = mkstemp(t); if (fd < 0) return -1; close(fd); return 0; }
int c_inotify(void) { int ?owned(close) fd = inotify_init1(0); if (fd < 0) return -1; int w = inotify_add_watch(fd, "/", 1); close(fd); return w; }
/* assert: a null test, its failure path does not return */
int c_assert(char *?p) { assert(p); return *p; }
static_assert(sizeof(int) == 4, "int is 32 bits");
/* the safe APIs */
int c_strl(char *d) { char b[8] = undefined; size_t n = strlcpy(b, "abc", sizeof b); n += strlcat(b, d, sizeof b); return cast<int(int)>(n + strlcpy(nullptr, d, 0)); }
void c_secret(void) { char k[16] = undefined; k[0] = 1; memset_explicit(k, 0, sizeof k); explicit_bzero(k, sizeof k); }
void c_reallocarray(int *?owned p) { int *?owned q = reallocarray(move(p), 8, sizeof(int)); if (!q) { free(p); return; } free(q); }
int c_ckd(int a, int b) { int r; if (ckd_add(&r, a, b) || ckd_mul(&r, r, 2) || ckd_sub(&r, r, 1)) return -1; return r; }
void t_getline(inout FILE *f) { char *?owned l = nullptr; size_t n = 0; while (getline(&l, &n, f) > 0) { } free(l); }
void t_memalign(void) { void *?owned m; if (posix_memalign(&m, 8, 64)) return; free(m); }
void t_addrinfo(void) { struct addrinfo *?owned r; if (getaddrinfo("h", "80", nullptr, &r)) return; freeaddrinfo(r); }
void t_regex(void) { regex_t owned(regfree) re = undefined; if (regcomp(&re, "a", 0)) return; regexec(&re, "x", 0, nullptr, 0); regfree(&re); }
void t_pipe(void) { int ?owned(close) p[2] = undefined; if (pipe(p) < 0) return; close(p[0]); close(p[1]); }
