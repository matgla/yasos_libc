/* yasc (-std=yasc -Wall): each line marked HIT warns, no other line does. */
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
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <langinfo.h>
#include <netdb.h>
#include <regex.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/inotify.h>
#include <netdb.h>
#include <regex.h>

int h_strchr(const char * view s) { const char * view p = strchr(s, 'x'); return *p; }            /* HIT store */
int h_strrchr(const char * view s) { char * view maybe p = strrchr(s, 'x'); return *p; }    /* HIT deref */
int h_strstr(const char * view s) { const char * view p = strstr(s, "ab"); return p != 0; }  /* HIT store */
int h_strpbrk(const char * view s) { char * view maybe p = strpbrk(s, "ab"); return *p; }   /* HIT deref */
int h_memchr(const char * view s) { const char * view maybe p = memchr(s, 'x', 4); return *p; } /* HIT deref */
int h_memrchr(const char * view s) { char * view maybe p = memrchr(s, 'x', 4); return *p; } /* HIT deref */
int h_strtok(char * view s) { char * view maybe t = strtok(s, " "); return *t; }           /* HIT deref */
int h_strtok_r(char * view s) { char * view maybe sp = 0; char * view maybe t = strtok_r(s, " ", &sp); return *t; } /* HIT deref */
int h_getenv(void) { char * view e = getenv("HOME"); return *e; }                          /* HIT store */
int h_getenv_arg(void) { char * view maybe e = getenv("A"); return getenv(e) != 0; }       /* HIT maybe arg */
int h_fgets(FILE * view f) { char buf[8]; char * view maybe l = fgets(buf, 8, f); return *l; } /* HIT deref */
int h_fgets_fp(void) { char buf[8]; FILE * owned maybe f = fopen("x", "r"); int r = fgets(buf, 8, f) != 0; if (f) fclose(f); return r; } /* HIT maybe arg */
int h_readdir(DIR * view d) { struct dirent * view maybe e = readdir(d); return e->d_type; } /* HIT deref */
int h_localtime(const time_t * view t) { struct tm * view maybe tm = localtime(t); return tm->tm_year; } /* HIT deref */
int h_gmtime_r(const time_t * view t, struct tm * view res) { struct tm * view r = gmtime_r(t, res); return r != 0; } /* HIT store */
int h_ctime(const time_t * view t) { char * view maybe s = ctime(t); return *s; }          /* HIT deref */
int h_strptime(struct tm * view tm) { char * view maybe e = strptime("1", "%d", tm); return *e; } /* HIT deref */
int h_getpwnam(void) { struct passwd * view maybe p = getpwnam("root"); return p->pw_uid; } /* HIT deref */
int h_getgrgid(void) { struct group * view maybe g = getgrgid(0); return g->gr_gid; }      /* HIT deref */
int h_setlocale(void) { char * view maybe l = setlocale(0, ""); return *l; }              /* HIT deref */
int h_dlsym(void) { void * view s = dlsym(0, "f"); return s != 0; }                      /* HIT store */
int h_dlerror(void) { char * view maybe e = dlerror(); return *e; }                       /* HIT deref */
int h_getmntent(FILE * view f) { struct mntent * view maybe m = getmntent(f); return m->mnt_freq; } /* HIT deref */
int h_hasmntopt(const struct mntent * view m) { char * view maybe o = hasmntopt(m, "ro"); return *o; } /* HIT deref */
int h_mkdtemp(char * view t) { char * view d = mkdtemp(t); return d != 0; }              /* HIT store */
int h_bsearch(const int * view k, const int * view a, int (*c)(const void * view, const void * view)) { const int * view maybe r = bsearch(k, a, 3, 4, c); return *r; } /* HIT deref */
int h_tmpnam(void) { char * view maybe n = tmpnam(0); return *n; }                        /* HIT deref */
/* -Wnonnull: a null constant */
int n_strtok(void) { return strtok(0, 0) != 0; }                 /* HIT nonnull */
int n_fopen(void) { FILE * owned maybe f = fopen(0, "r"); if (f) fclose(f); return 0; } /* HIT nonnull */
int n_printf(void) { return printf(0); }                         /* HIT nonnull */
int n_snprintf(void) { char b[4]; return snprintf(b, 4, 0); }    /* HIT nonnull */
int n_strtol(void) { return (int)strtol(0, 0, 10); }             /* HIT nonnull */
int n_qsort(int * view a) { qsort(a, 1, 4, 0); return 0; }       /* HIT nonnull */
int n_localtime(void) { return localtime(0) != 0; }              /* HIT nonnull */
void t_realloc_back(char * owned maybe p) { p = realloc(move(p), 8); free(p); }       /* HIT takes(v): p may leak */
int t_fd_untested(void) { int owned(close) maybe fd = open("/x", O_RDONLY); int r = (int)read(fd, 0, 0); if (fd >= 0) close(fd); return r; } /* HIT maybe handle to int view */
void t_fd_plain(void) { int fd = open("/x", O_RDONLY); (void)fd; }        /* HIT owned result in a plain int */
/* borrows: a result that points into an argument cannot leave as borrows(static) */
char * borrows(static) b_dirname(char * view p) { return dirname(p); }          /* HIT borrows(static, path) */
char * borrows(static) b_basename(char * view p) { return basename(p); }        /* HIT borrows(static, path) */
char * borrows(static) b_asctime_r(const struct tm * view t, char * view b) { return asctime_r(t, b); } /* HIT borrows(buf) */
char * borrows(static) b_strcpy(char * view d) { return strcpy(d, "x"); }       /* HIT borrows(dst) */
char * borrows(static) b_strcat(char * view d) { return strcat(d, "x"); }       /* HIT borrows(dst) */
char * borrows(static) b_stpcpy(char * view d) { return stpcpy(d, "x"); }       /* HIT borrows(dst) */
char * borrows(static) b_itoa(char * view s) { return itoa(1, s, 10); }         /* HIT borrows(s) */
const char * borrows(static) maybe b_inet_ntop(const void * view s, char * view d) { return inet_ntop(AF_INET, s, d, 16); } /* HIT borrows(dst) */
int b_getlogin(void) { char * view maybe l = getlogin(); return *l; }          /* HIT deref */
/* handles */
void t_accept_dropped(int view l) { accept(l, 0, 0); }                         /* HIT owned result discarded */
int t_accept_maybe(void) { int owned(close) maybe s = socket(AF_INET, 1, 0); int owned(close) maybe c = accept(s, 0, 0); if (c >= 0) close(c); if (s >= 0) close(s); return 0; } /* HIT maybe handle to int view */
int t_dup2_maybe(int owned(close) maybe fd) { int r = dup2(fd, 1); close(fd); return r; } /* HIT maybe handle to int view */
void t_mkstemp(char * view t) { mkstemp(t); }                                  /* HIT owned result discarded */
void t_inotify(void) { int fd = inotify_init(); (void)fd; }                    /* HIT owned result in a plain int */
int t_inotify_watch(void) { int owned(close) maybe fd = inotify_init1(0); int w = inotify_add_watch(fd, "/", 1); if (fd >= 0) close(fd); return w; } /* HIT maybe handle to int view */
/* the safe APIs */
int s_strlcpy_maybe(void) { char b[8]; char * view maybe s = getenv("A"); return (int)strlcpy(b, s, 8); } /* HIT maybe arg */
int s_strlcat_maybe(char * view d) { return (int)strlcat(d, getenv("A"), 8); }  /* HIT maybe arg */
void * borrows(static) maybe s_memset_explicit(char * view b) { return memset_explicit(b, 0, 4); } /* HIT borrows(s) */
void s_reallocarray_back(int * owned maybe p) { p = reallocarray(move(p), 4, 4); free(p); } /* HIT takes(p): p may leak */
void s_reallocarray_dropped(void) { reallocarray(0, 2, 2); }                  /* HIT owned result discarded */
/* assert: with NDEBUG (read again at each inclusion) it tests nothing */
#define NDEBUG
#include <assert.h>
int h_assert_off(char * view maybe p) { assert(p); return *p; }                /* HIT deref */
#undef NDEBUG
#include <assert.h>
int h_assert_on(char * view maybe p) { assert(p != 0); return *p; }
void t_getline_leak(FILE * view f) { char * owned maybe l = 0; size_t n = 0; getline(&l, &n, f); }            /* HIT out-param: l still owns the buffer */
void t_memalign_twice(void) { void * owned maybe m = 0; if (posix_memalign(&m, 8, 8)) return; if (posix_memalign(&m, 8, 16)) return; free(m); } /* HIT out: m overwritten */
void t_regex_leak(void) { regex_t owned(regfree) re; if (regcomp(&re, "a", 0)) return; regexec(&re, "x", 0, 0, 0); } /* HIT re never released */
void t_regex_plain(void) { regex_t re; regcomp(&re, "a", 0); }                        /* HIT not declared owned */
void t_pipe_half(void) { int owned(close) maybe p[2]; if (pipe(p) < 0) return; close(p[0]); } /* HIT p[1] leaks */
