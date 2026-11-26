#pragma yasc
#pragma tcc c_begin
// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/_attrs.h>

/* C requires these in <stdio.h> as well as POSIX's <unistd.h>. */
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define EOF (-1)

#define _IONBF 1
#define _IOLBF 2
#define _IOFBF 0

/* standard file descriptors */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define P_tmpdir "/tmp"
#define L_tmpnam 32
#define TMP_MAX 16384

#define putc(c, fp) (fputc(c, fp))
#define getc(fp) (fgetc(fp))

typedef struct {
  int fd;
  int back;          /* pushback buffer */
  char *ibuf, *obuf; /* input/output buffer */
  int isize, osize;  /* ibuf size */
  int ilen, olen;    /* length of data in buf */
  int iown, oown;    /* free the buffer when finished */
  int icur;          /* current position in ibuf */
  int ostat;
  int istat;         /* _FILE_EOF / _FILE_ERR: what feof()/ferror() report */
} FILE;

#define STDIO_OBUF_LAZY (-1) /* oown: allocate at the first write */
#define _FILE_EOF 1 /* a read found the end of the input */
#define _FILE_ERR 2 /* a read or write failed */


/* Embedded target: 256 B is plenty for line/block buffering and keeps the
 * three static stream buffers (stdin/stdout/stderr) + every fopen buffer small.
 * Trade: slightly more read/write syscalls on bulk I/O. */
#define BUFSIZ 256

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;


/* fclose first: fopen names it as its deallocator (sys/_attrs.h) */
int fclose(FILE *fp);
FILE *fopen(const char *path, const char *mode) __yl_dealloc(fclose, 1) __yl_nonnull(1, 2);
/* not an allocator: it hands back FP itself (or NULL, FP closed), and the usual
   freopen(path, mode, stdin) ignores the result */
FILE *freopen(const char *path, const char *mode, FILE *fp) __yl_nonnull(2, 3);
int fflush(FILE *fp);
void setbuf(FILE *fp, char *buf) __yl_nonnull(1);

FILE *tmpfile(void) __yl_dealloc(fclose, 1);
#pragma tcc c_end
/* BUFFER or a static one; null when no name is left */
char * borrows(static, buffer) maybe tmpnam(out char * view maybe buffer);
#pragma tcc c_begin
int fputc(int c, FILE *fp) __yl_nonnull(2);
int putchar(int c);
int printf(const char *fmt, ...) __yl_nonnull(1);
int vprintf(const char *fmt, va_list ap) __yl_nonnull(1);
int fprintf(FILE *fp, const char *fmt, ...) __yl_nonnull(1, 2);
int sprintf(char *dst, const char *fmt, ...) __yl_nonnull(1, 2);
int vsprintf(char *dst, const char *fmt, va_list ap) __yl_nonnull(1, 2);
int vfprintf(FILE *fp, const char *fmt, va_list ap) __yl_nonnull(1, 2);
int snprintf(char *dst, size_t sz, const char *fmt, ...) __yl_nonnull(3);
int vsnprintf(char *dst, size_t sz, const char *fmt, va_list ap) __yl_nonnull(3);
int fputs(const char *s, FILE *fp) __yl_nonnull(1, 2);
int ferror(FILE *stream) __yl_nonnull(1);
int puts(const char *s) __yl_nonnull(1);
int pclose(FILE *stream);
int fseek(FILE *fp, long offset, int whence) __yl_nonnull(1);
long ftell(FILE *fp) __yl_nonnull(1);
typedef long fpos_t;
void rewind(FILE *fp) __yl_nonnull(1);
int fgetpos(FILE *fp, fpos_t *pos) __yl_nonnull(1, 2);
int fsetpos(FILE *fp, const fpos_t *pos) __yl_nonnull(1, 2);
FILE *popen(const char *command, const char *type) __yl_dealloc(pclose, 1) __yl_nonnull(1, 2);

int fgetc(FILE *fp) __yl_nonnull(1);
#pragma tcc c_end
/* S, or null at the end of the input or on an error */
char * borrows(s) maybe fgets(out char * view s, int sz, inout FILE * view fp) __yl_nonnull(1, 3);
#pragma tcc c_begin
int scanf(const char *fmt, ...) __yl_nonnull(1);
int fscanf(FILE *fp, const char *fmt, ...) __yl_nonnull(1, 2);
int sscanf(const char *s, const char *fmt, ...) __yl_nonnull(1, 2);
int vsscanf(const char *s, const char *fmt, va_list ap) __yl_nonnull(1, 2);
int vfscanf(FILE *fp, const char *fmt, va_list ap) __yl_nonnull(1, 2);
int getchar(void);
int ungetc(int c, FILE *fp) __yl_nonnull(2);
long fwrite(void *s, long sz, long n, FILE *fp) __yl_nonnull(4);
long fread(void *s, long sz, long n, FILE *fp) __yl_nonnull(4);
int feof(FILE *fp) __yl_nonnull(1);

void perror(const char *s);

#pragma tcc c_end
/* the line buffer is the caller's before and after (the callee may replace
   it), and must be freed even after -1 */
ssize_t getline(inout char * owned(free) maybe * view lineptr, inout size_t * view n, inout FILE * view fp) __yl_nonnull(3);
#pragma tcc c_begin
int setvbuf(FILE *fp, char *buf, int mode, size_t size) __yl_nonnull(1);
int dprintf(int fildes, const char *format, ...) __yl_nonnull(2);
int vdprintf(int fildes, const char *format, va_list ap) __yl_nonnull(2);

int fileno(FILE *fp) __yl_nonnull(1);

int rename(const char *oldpath, const char *newpath) __yl_nonnull(1, 2);
int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath) __yl_nonnull(2, 4);
int renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath,
              unsigned int flags) __yl_nonnull(2, 4);

/* renameat2 flags, same values as Linux. Only NOREPLACE is honoured. */
#define RENAME_NOREPLACE (1 << 0)
#define RENAME_EXCHANGE (1 << 1)
#define RENAME_WHITEOUT (1 << 2)
#pragma tcc c_end
/* the stream takes the descriptor over; null: fd is still the caller's */
FILE * owned(fclose) maybe takes(fd) fdopen(int owned(close) fd, const char * view mode) __yl_nonnull(2);
#pragma tcc c_begin
FILE *fmemopen(void *buf, size_t size, const char *mode) __yl_dealloc(fclose, 1) __yl_nonnull(3);
#pragma tcc c_end
ssize_t getdelim(inout char * owned(free) maybe * view lineptr, inout size_t * view n, int delimiter,
                 inout FILE * view stream) __yl_nonnull(4);
#pragma tcc c_begin
void clearerr(FILE *fp) __yl_nonnull(1);

int remove(const char *pathname) __yl_nonnull(1);
/* _FORTIFY_SOURCE (sys/_fortify.h): the printf functions that know their
   destination's size, and the checking functions they call when the compiler
   cannot prove the output fits (fortify.c; glibc's names and arguments) */
#include <sys/_fortify.h>
#ifdef __YL_FORTIFY
int __sprintf_chk(char *dst, int flag, size_t dstlen, const char *fmt, ...);
int __snprintf_chk(char *dst, size_t sz, int flag, size_t dstlen, const char *fmt, ...);
int __vsprintf_chk(char *dst, int flag, size_t dstlen, const char *fmt, va_list ap);
int __vsnprintf_chk(char *dst, size_t sz, int flag, size_t dstlen, const char *fmt, va_list ap);

#define sprintf(d, ...) __builtin___sprintf_chk(d, __yl_fortify_flag, __yl_bos(d), __VA_ARGS__)
#define snprintf(d, n, ...) __builtin___snprintf_chk(d, n, __yl_fortify_flag, __yl_bos(d), __VA_ARGS__)
#define vsprintf(d, f, ap) __builtin___vsprintf_chk(d, __yl_fortify_flag, __yl_bos(d), f, ap)
#define vsnprintf(d, n, f, ap) __builtin___vsnprintf_chk(d, n, __yl_fortify_flag, __yl_bos(d), f, ap)
#endif
#pragma tcc c_end
