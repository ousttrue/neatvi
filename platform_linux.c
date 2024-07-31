#include "platform.h"

static struct termios termios;

void term_init(void) {
  struct winsize win;
  struct termios newtermios;
  tcgetattr(0, &termios);
  newtermios = termios;
  newtermios.c_lflag &= ~(ICANON | ISIG);
  newtermios.c_lflag &= ~ECHO;
  tcsetattr(0, TCSAFLUSH, &newtermios);
  if (getenv("LINES"))
    rows = atoi(getenv("LINES"));
  if (getenv("COLUMNS"))
    cols = atoi(getenv("COLUMNS"));
  if (!ioctl(0, TIOCGWINSZ, &win)) {
    cols = win.ws_col;
    rows = win.ws_row;
  }
  cols = cols ? cols : 80;
  rows = rows ? rows : 25;
  term_str("\33[m");
  term_window(win_beg, win_rows > 0 ? win_rows : rows);
}

void term_done(void) {
  term_str("\33[r");
  term_pos(rows - 1, 0);
  term_kill();
  term_commit();
  tcsetattr(0, 0, &termios);
}

void term_suspend(void) {
  term_done();
  kill(0, SIGSTOP);
  term_init();
}

int term_read(void) {
  struct pollfd ufds[1];
  int n, c;
  if (ibuf_pos >= ibuf_cnt) {
    ufds[0].fd = 0;
    ufds[0].events = POLLIN;
    if (poll(ufds, 1, -1) <= 0)
      return -1;
    /* read a single input character */
    if ((n = read(0, ibuf, 1)) <= 0)
      return -1;
    ibuf_cnt = n;
    ibuf_pos = 0;
  }
  c = ibuf_pos < ibuf_cnt ? (unsigned char)ibuf[ibuf_pos++] : -1;
  if (icmd_pos < sizeof(icmd))
    icmd[icmd_pos++] = c;
  return c;
}

int cmd_make(char **argv, int *ifd, int *ofd) {
  int pid;
  int pipefds0[2];
  int pipefds1[2];
  if (ifd)
    pipe(pipefds0);
  if (ofd)
    pipe(pipefds1);
  if (!(pid = fork())) {
    if (ifd) { /* setting up stdin */
      close(0);
      dup(pipefds0[0]);
      close(pipefds0[1]);
      close(pipefds0[0]);
    }
    if (ofd) { /* setting up stdout and stderr */
      close(1);
      dup(pipefds1[1]);
      close(2);
      dup(pipefds1[1]);
      close(pipefds1[0]);
      close(pipefds1[1]);
    }
    execvp(argv[0], argv);
    exit(1);
  }
  if (ifd)
    close(pipefds0[0]);
  if (ofd)
    close(pipefds1[1]);
  if (pid < 0) {
    if (ifd)
      close(pipefds0[1]);
    if (ofd)
      close(pipefds1[0]);
    return -1;
  }
  if (ifd)
    *ifd = pipefds0[1];
  if (ofd)
    *ofd = pipefds1[0];
  return pid;
}

/*
 * Execute a shell command.
 *
 * If ibuf is given, it is passed as standard input to the process.
 * Otherwise, the process reads from the terminal.
 *
 * If oproc is 0, the process writes directly to the terminal.  If it
 * is 1, process' output is saved and returned.  If it is 2, in addition
 * to returning the output, it is written to the terminal.
 */
char *cmd_pipe(char *cmd, char *ibuf, int oproc) {
  char *argv[] = {"/bin/sh", "-c", cmd, NULL};
  struct pollfd fds[3];
  struct sbuf *sb = NULL;
  char buf[512];
  int ifd = -1, ofd = -1;
  int slen = ibuf != NULL ? strlen(ibuf) : 0;
  int nw = 0;
  int pid = cmd_make(argv, ibuf != NULL ? &ifd : NULL, oproc ? &ofd : NULL);
  if (pid <= 0)
    return NULL;
  if (oproc)
    sb = sbuf_make();
  if (ibuf == NULL) {
    signal(SIGINT, SIG_IGN);
    term_done();
  }
  fcntl(ifd, F_SETFL, fcntl(ifd, F_GETFL, 0) | O_NONBLOCK);
  fds[0].fd = ofd;
  fds[0].events = POLLIN;
  fds[1].fd = ifd;
  fds[1].events = POLLOUT;
  fds[2].fd = isatty(0) && ibuf != NULL ? 0 : -1;
  fds[2].events = POLLIN;
  while ((fds[0].fd >= 0 || fds[1].fd >= 0) && poll(fds, 3, 200) >= 0) {
    if (fds[0].revents & POLLIN) {
      int ret = read(fds[0].fd, buf, sizeof(buf));
      if (ret > 0 && oproc == 2)
        write(1, buf, ret);
      if (ret > 0)
        sbuf_mem(sb, buf, ret);
      if (ret <= 0) {
        close(fds[0].fd);
        fds[0].fd = -1;
      }
    } else if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
      close(fds[0].fd);
      fds[0].fd = -1;
    }
    if (fds[1].revents & POLLOUT) {
      int ret = write(fds[1].fd, ibuf + nw, slen - nw);
      if (ret > 0)
        nw += ret;
      if (ret <= 0 || nw == slen) {
        close(fds[1].fd);
        fds[1].fd = -1;
      }
    } else if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
      close(fds[1].fd);
      fds[1].fd = -1;
    }
    if (fds[2].revents & POLLIN) {
      int ret = read(fds[2].fd, buf, sizeof(buf));
      int i;
      for (i = 0; i < ret; i++)
        if ((unsigned char)buf[i] == TK_CTL('c'))
          kill(pid, SIGINT);
    } else if (fds[2].revents & (POLLERR | POLLHUP | POLLNVAL)) {
      fds[2].fd = -1;
    }
  }
  close(fds[0].fd);
  close(fds[1].fd);
  waitpid(pid, NULL, 0);
  if (ibuf == NULL) {
    term_init();
    signal(SIGINT, SIG_DFL);
  }
  if (oproc)
    return sbuf_done(sb);
  return NULL;
}
