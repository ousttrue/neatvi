#pragma once

#ifdef _WIN32
#else
#include <poll.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

int cmd_make(char **argv, int *ifd, int *ofd);
