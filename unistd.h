#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>

/* access() flags */
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0

int access(char *name, int type);
int unlink(const char *path);

extern char **environ;

int isatty(int fd);
int close(int fd);

ssize_t write(int fd, const void *buf, size_t n);
ssize_t read(int fd, void *buf, size_t n);

/* lseek() whence */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

long lseek(int fd, long offset, int whence);

int pipe(int fds[2]);
int dup(int fd);
int dup2(int fd, int fd2);

int vfork(void);
int fork(void);
int getpid(void);
int getppid(void);

int execve(const char *path, char *const argv[], char *const envp[]);
int execle(char *path, ...);
int execvp(char *file, char *argv[]);
int execv(char *path, char *argv[]);

void *sbrk(intptr_t increment);

void _exit(int status);

int sleep(int n);

char *getcwd(char *buf, size_t size);
int chdir(const char *path);
