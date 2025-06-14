#pragma once

#include <stdarg.h>
#include <stddef.h>

#define EOF (-1)

/* standard file descriptors */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

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
  int istat;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *fp);
int fflush(FILE *fp);
void setbuf(FILE *fp, char *buf);

int fputc(int c, FILE *fp);
int putchar(int c);
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int fprintf(FILE *fp, const char *fmt, ...);
int sprintf(char *dst, const char *fmt, ...);
int vsprintf(char *dst, const char *fmt, va_list ap);
int vfprintf(FILE *fp, const char *fmt, va_list ap);
int snprintf(char *dst, int sz, const char *fmt, ...);
int vsnprintf(char *dst, int sz, const char *fmt, va_list ap);
int fputs(const char *s, FILE *fp);
int puts(const char *s);

int fgetc(FILE *fp);
char *fgets(char *s, int sz, FILE *fp);
int scanf(char *fmt, ...);
int fscanf(FILE *fp, char *fmt, ...);
int sscanf(char *s, char *fmt, ...);
int vsscanf(char *s, char *fmt, va_list ap);
int vfscanf(FILE *fp, char *fmt, va_list ap);
int getchar(void);
int ungetc(int c, FILE *fp);
long fwrite(void *s, long sz, long n, FILE *fp);
long fread(void *s, long sz, long n, FILE *fp);

void perror(char *s);

int getline(char **lineptr, size_t *n, FILE *fp);