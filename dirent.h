#pragma once

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

DIR *opendir(const char *path);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dir);

DIR *fdopendir(int fd);
