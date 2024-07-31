#pragma once

extern int rows, cols;		/* number of terminal rows and columns */
extern int win_beg, win_rows;	/* active window rows */
extern int ibuf_pos, ibuf_cnt;	/* ibuf[] position and length */
extern char ibuf[4096];		/* input character buffer */
extern int icmd_pos;		/* icmd[] position */
extern char icmd[4096];		/* read after the last term_cmd() */

int cmd_make(char **argv, int *ifd, int *ofd);
