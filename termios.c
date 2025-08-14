// Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
// Please check the LICENSE file for copying conditions.
// Modified by:
// Copyright (c) 2025 Mateusz Stadnik <matgla@live.com>

#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <stddef.h>

#include <stdio.h>

int tcgetattr(int fd, struct termios *term) {
  return ioctl(fd, TCGETS, term);
}

int tcsetattr(int fd, int actions, struct termios *term) {
  switch (actions) {
  case TCSANOW:
    return ioctl(fd, TCSETS, term);
  case TCSADRAIN:
    return ioctl(fd, TCSETSW, term);
  case TCSAFLUSH:
    return ioctl(fd, TCSETSF, term);
  }
  errno = EINVAL;
  return -1;
}

void cfmakeraw(struct termios *t) {
  t->c_iflag &=
      ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  t->c_oflag &= ~OPOST;
  t->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  t->c_cflag &= ~(CSIZE | PARENB);
  t->c_cflag |= CS8;
  t->c_cc[VMIN] = 1;
  t->c_cc[VTIME] = 0;
}

int tcflush(int fildes, int queue_selector) {
  printf("TODO: Implement tcflush\n");
  return -1; // Not implemented
}

int tcdrain(int fd) {
  printf("TODO: Implement tcdrain\n");
  return -1; // Not implemented
}
int tcflow(int fd, int action) {
  printf("TODO: Implement tcflow\n");
  return -1; // Not implemented
}

speed_t cfgetispeed(const struct termios *termios_p) {
  printf("TODO: Implement cfgetispeed\n");
  return 0;
}

speed_t cfgetospeed(const struct termios *termios_p) {
  printf("TODO: Implement cfgetospeed\n");
  return 0;
}

int cfsetispeed(struct termios *termios_p, speed_t speed) {
  printf("TODO: Implement cfsetispeed\n");
  return 0;
}

int cfsetospeed(struct termios *termios_p, speed_t speed) {
  printf("TODO: Implement cfsetospeed\n");
  return 0;
}

int cfsetspeed(struct termios *termios_p, speed_t speed) {
  printf("TODO: Implement cfsetspeed\n");
  return 0;
}
