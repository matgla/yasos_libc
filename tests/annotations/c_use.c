/* Regular C (-ccompat -Wall): the correct calls report nothing, each null
   constant marked HIT is a -Wnonnull warning. */
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
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <langinfo.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <sys/stat.h>

static int cmp(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
int ok(const char *s, char *w, FILE *f) {
  char buf[16];
  const char *p = strchr(s, 'x');
  const char *q = strstr(s, "ab");
  const void *m = memchr(s, 'x', 4);
  char *e = getenv("HOME");
  char *sp = 0;
  char *t = strtok_r(w, " ", &sp);
  char *l = fgets(buf, sizeof buf, f);
  DIR *d = opendir("/");
  struct dirent *de = d ? readdir(d) : 0;
  time_t now = time(0);
  struct tm *tm = localtime(&now);
  struct passwd *pw = getpwuid(0);
  struct group *gr = getgrgid(0);
  char *loc = setlocale(0, 0);
  void *sym = dlsym(0, "f");
  int a[3] = {3, 1, 2}, k = 2;
  qsort(a, 3, sizeof a[0], cmp);
  int *r = bsearch(&k, a, 3, sizeof a[0], cmp);
  struct stat st;
  int fd = open("/x", O_RDONLY);
  if (d) closedir(d);
  fflush(0);
  setvbuf(stdout, 0, _IONBF, 0);
  perror(0);
  return (p != 0) + (q != 0) + (m != 0) + (e != 0) + (t != 0) + (l != 0) + (de != 0) + (tm != 0) + (pw != 0) +
         (gr != 0) + (loc != 0) + (sym != 0) + (r != 0) + snprintf(0, 0, "%d", 1) + (int)strtol(s, 0, 10) +
         stat("/x", &st) + access("/x", 0) + fd + strcasecmp(s, "a") + (getcwd(0, 0) != 0) +
         (realpath("/", 0) != 0) + setenv("A", 0, 1) + (int)getline(0, 0, f) + (int)memchr(0, 0, 0);
}
int views(char *p, const struct tm *t, char *b) {
  char tmpl[] = "/tmp/xXXXXXX";
  int fd = mkstemp(tmpl);
  int c = accept(fd, 0, 0);
  if (c >= 0) close(c);
  if (fd >= 0) close(fd);
  return *dirname(p) + *basename(0) + *strerror(1) + *asctime(t) + *asctime_r(t, b) + *nl_langinfo(CODESET) +
         *gai_strerror(0) + *strcpy(b, p) + *itoa(1, b, 10) + (getlogin() != 0) + dup2(0, 1) + inotify_init();
}
int asserts(const char *s) {
  assert(s && *s);
  assert((int[]){1, 2}[1] == 2);
  return *s;
}
static_assert(sizeof(long) == 4, "ILP32");
int safe_apis(char *d, int *p) {
  char k[8];
  int r;
  size_t n = strlcpy(k, d, sizeof k) + strlcat(k, d, sizeof k) + strlcpy(0, d, 0);
  memset_explicit(k, 0, sizeof k);
  explicit_bzero(k, sizeof k);
  int *q = reallocarray(p, 4, sizeof *p);
  if (!q) free(p);
  else free(q);
  return (int)n + (ckd_add(&r, 1, 2) ? 0 : r);
}
int nulls(void) {
  int n = 0;
  n += printf(0); /* HIT */
  n += (strtok(0, 0) != 0); /* HIT */
  n += (fopen(0, "r") != 0); /* HIT */
  n += atoi(0); /* HIT */
  n += stat(0, 0); /* HIT */
  n += open(0, 0); /* HIT */
  n += (localtime(0) != 0); /* HIT */
  n += strcasecmp(0, "a"); /* HIT */
  n += (getenv(0) != 0); /* HIT */
  n += rename("a", 0); /* HIT */
  n += fputs("x", 0); /* HIT */
  n += inotify_add_watch(0, 0, 1); /* HIT */
  n += (int)strlcpy(0, 0, 0); /* HIT */
  return n;
}
