#pragma once

#include <sys/_attrs.h>

typedef struct __dirent_dir DIR;

#define DT_BLK 1
#define DT_CHR 2
#define DT_DIR 3
#define DT_FIFO 4
#define DT_LNK 5
#define DT_REG 6
#define DT_SOCK 7
#define DT_UNKNOWN 0

struct dirent {
  unsigned long d_ino;
  unsigned long d_off;
  unsigned short d_reclen;
  unsigned char d_type;
  char d_name[256];
};

/* yasc's reading of the results that may be null (sys/_attrs.h) */
#include <sys/yasc/dirent.h>

/* closedir first: opendir names it as its deallocator (sys/_attrs.h) */
int closedir(DIR *dir);
DIR *opendir(const char *path) __yl_dealloc(closedir, 1) __yl_nonnull(1);
struct dirent *readdir(DIR *dir) __yl_nonnull(1);

DIR *fdopendir(int fd) __yl_dealloc(closedir, 1);
