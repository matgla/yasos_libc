#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

struct __dirent_dir {
  int owned(close) maybe fd;
  int buf_pos;
  int buf_end;
  /* getdents scratch. 1 KiB still batches several entries per syscall and keeps
   * the whole DIR under MSETMAX so it stays in the malloc pool (no dedicated
   * page) instead of a direct mmap. */
  char buf[1024];
};

DIR *owned(closedir) maybe opendir(const char *path)
{ 
  int owned(close) maybe fd = 0;
  if ((fd = open(path, O_RDONLY | O_DIRECTORY)) < 0)
    return nullptr;
  fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
  
  DIR *owned(free) maybe dir = malloc(sizeof(*dir));
  if (!dir) {
    close(fd);
    return nullptr;
  }

  memset(borrow(dir), 0, sizeof(*dir));
  dir->fd = move(fd);
  return dir;
}

int closedir(drop DIR *owned maybe dir) {
  if (!dir) {
    errno = EBADF;
    return -1;
  }
  int ret;
  ret = close(dir->fd);
  free(dir);
  return ret;
}

int getdents(int view fd, struct dirent *de, size_t len);

struct dirent *borrows(dir) maybe readdir(inout DIR *dir) {
  int len;
  if (dir->buf_pos >= dir->buf_end) {
    if (dir->fd < 0) {
      errno = EBADF;
      return nullptr;
    }
    len = getdents(dir->fd, cast<ptr(struct dirent *view)>(dir->buf), sizeof(dir->buf));
    if (len <= 0)
      return nullptr;
    dir->buf_pos = 0;
    dir->buf_end = len;
  }
  struct dirent *de = cast<ptr(struct dirent *), align(4)>(dir->buf + dir->buf_pos);
  dir->buf_pos += de->d_reclen;
  return de;
}

DIR *owned(closedir) maybe takes(fd) fdopendir(int owned(close) fd) {
  fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
  DIR *owned(free) maybe dir = malloc(sizeof(*dir));
  if (!dir) {
    return nullptr;
  }
  memset(borrow(dir), 0, sizeof(*dir));
  dir->fd = move(fd);
  return dir;
}