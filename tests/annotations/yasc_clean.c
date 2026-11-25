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

static int cmp(const void * view a, const void * view b) { return *(const int * view)a - *(const int * view)b; }
int c_strchr(const char * view s) { const char * view maybe p = strchr(s, 'x'); if (!p) return 0; return *p; }
int c_strstr(const char * view s) { return strstr(s, "ab") ? 1 : 0; }
int c_memchr(const char * view s) { const char * view maybe p = memchr(s, 'x', 4); return p ? *p : 0; }
int c_memchr_null(void) { return memchr(0, 'x', 0) != 0; }
int c_strtok(char * view s) { int n = 0; for (char * view maybe t = strtok(s, " "); t; t = strtok(0, " ")) n += *t; return n; }
int c_strtok_r(char * view s) { char * view maybe sp = 0; char * view maybe t = strtok_r(s, " ", &sp); return t ? *t : 0; }
int c_getenv(void) { char * view maybe e = getenv("HOME"); if (!e) return 0; return *e + (int)strlen(e); }
int c_fgets(FILE * view f) { char buf[8] = undefined; while (fgets(buf, 8, f)) if (buf[0] == 'x') return 1; return 0; }
int c_fopen(void) { char buf[8] = undefined; FILE * owned maybe f = fopen("x", "r"); if (!f) return -1; int r = fgets(buf, 8, f) != 0; fclose(f); return r; }
int c_readdir(void) { int n = 0; DIR * owned maybe d = opendir("/"); if (!d) return -1; struct dirent * view maybe e; while ((e = readdir(d))) n += e->d_type; closedir(d); return n; }
int c_localtime(void) { time_t t = time(0); struct tm * view maybe tm = localtime(&t); return tm ? tm->tm_year : 0; }
int c_ctime_r(const time_t * view t) { char b[26] = undefined; char * view maybe s = ctime_r(t, b); return s ? *s : 0; }
int c_getpw(void) { struct passwd * view maybe p = getpwuid(0); if (!p) return -1; return (int)p->pw_uid; }
int c_getgr(void) { struct group * view maybe g = getgrnam("wheel"); return g ? (int)g->gr_gid : -1; }
int c_setlocale(void) { char * view maybe l = setlocale(0, 0); return l ? *l : 0; }
int c_dl(void) { void * view maybe s = dlsym(0, "f"); char * view maybe e = dlerror(); return (s != 0) + (e ? *e : 0); }
int c_mnt(void) { FILE * owned maybe f = setmntent("/etc/fstab", "r"); if (!f) return -1; int n = 0; struct mntent * view maybe m; while ((m = getmntent(f))) n += hasmntopt(m, "ro") != 0; endmntent(f); return n; }
int c_bsearch(int k) { static const int a[3] = {1, 2, 3}; const int * view maybe r = bsearch(&k, a, 3, sizeof a[0], cmp); return r ? *r : -1; }
int c_qsort(int * view a) { qsort(a, 3, sizeof a[0], cmp); return a[0]; }
int c_tmpnam(void) { char * view maybe n = tmpnam(0); return n ? *n : 0; }
int c_nulls(void) {
  /* null where it is allowed */
  char b[4] = undefined;
  fflush(0); setvbuf(stdout, 0, _IONBF, 0); perror(0);
  return snprintf(0, 0, "%d", 1) + (int)strtol("1", 0, 10) + (int)time(0) + (getcwd(0, 0) != 0) +
         (realpath("/", 0) != 0) + setenv("A", 0, 1) + (int)sizeof b;
}
int c_paths(void) { struct stat st = undefined; int owned(close) maybe fd = open("/x", O_RDONLY); if (fd >= 0) close(fd); return stat("/x", &st) + access("/x", 0) + unlink("/y"); }
void t_realloc(char * owned maybe p) { char * owned maybe q = realloc(move(p), 8); if (!q) { free(p); return; } free(q); }
const char * borrows(static) c_strerror(int e) { char * view s = strerror(e); return *s ? s : strerror(0); }
char * borrows(static) c_asctime(const struct tm * view t) { return asctime(t); }
char * borrows(b) c_asctime_r(const struct tm * view t, char * view b) { return asctime_r(t, b); }
char * borrows(static, p) c_dirname(char * view maybe p) { return dirname(p); }
int c_basename(char * view p) { char * view b = basename(p); return *b + *dirname(0) + *basename(0); }
char * borrows(static) c_langinfo(void) { return nl_langinfo(CODESET); }
const char * borrows(static) c_gai(int e) { return gai_strerror(e); }
char * borrows(d) c_strcpy(char * view d, const char * view s) { strcpy(d, s); strcat(d, s); return stpcpy(d, s); }
char * borrows(s) c_itoa(char * view s) { return itoa(1, s, 10); }
const char * borrows(d) maybe c_inet_ntop(const void * view s, char * view d) { return inet_ntop(AF_INET, s, d, 16); }
int c_getlogin(void) { char * view maybe l = getlogin(); return l ? *l : 0; }
int c_accept(void) { int owned(close) maybe s = socket(AF_INET, 1, 0); if (s < 0) return -1; if (listen(s, 1)) { close(s); return -1; } int owned(close) maybe c = accept(s, 0, 0); if (c >= 0) close(c); close(s); return 0; }
/* a plain int (pipe's, until owned out-parameters) and a constant */
int c_dup2(int fd) { return dup2(fd, 1) + dup2(0, 2); }
int c_dup2_handle(int owned(close) maybe fd) { if (fd < 0) return -1; int r = dup2(fd, 1); close(fd); return r; }
int c_mkstemp(char * view t) { int owned(close) maybe fd = mkstemp(t); if (fd < 0) return -1; close(fd); return 0; }
int c_inotify(void) { int owned(close) maybe fd = inotify_init1(0); if (fd < 0) return -1; int w = inotify_add_watch(fd, "/", 1); close(fd); return w; }
/* assert: a null test, its failure path does not return */
int c_assert(char * view maybe p) { assert(p); return *p; }
static_assert(sizeof(int) == 4, "int is 32 bits");
/* the safe APIs */
int c_strl(char * view d) { char b[8] = undefined; size_t n = strlcpy(b, "abc", sizeof b); n += strlcat(b, d, sizeof b); return (int)(n + strlcpy(0, d, 0)); }
void c_secret(void) { char k[16] = undefined; k[0] = 1; memset_explicit(k, 0, sizeof k); explicit_bzero(k, sizeof k); }
void c_reallocarray(int * owned maybe p) { int * owned maybe q = reallocarray(move(p), 8, sizeof(int)); if (!q) { free(p); return; } free(q); }
int c_ckd(int a, int b) { int r; if (ckd_add(&r, a, b) || ckd_mul(&r, r, 2) || ckd_sub(&r, r, 1)) return -1; return r; }
void t_getline(FILE * view f) { char * owned maybe l = 0; size_t n = 0; while (getline(&l, &n, f) > 0) { } free(l); }
void t_memalign(void) { void * owned maybe m; if (posix_memalign(&m, 8, 64)) return; free(m); }
void t_addrinfo(void) { struct addrinfo * owned maybe r; if (getaddrinfo("h", "80", 0, &r)) return; freeaddrinfo(r); }
void t_regex(void) { regex_t owned(regfree) re = undefined; if (regcomp(&re, "a", 0)) return; regexec(&re, "x", 0, 0, 0); regfree(&re); }
void t_pipe(void) { int owned(close) maybe p[2] = undefined; if (pipe(p) < 0) return; close(p[0]); close(p[1]); }
