#include "platform.h"
#include <Windows.h>
#include <stdio.h>
#include <sys/types.h>

struct Stream {
  DWORD Mode;
  HANDLE Handle;
};
void InitRawInput(struct Stream *s) {
  s->Handle = GetStdHandle(STD_INPUT_HANDLE);
  if (!GetConsoleMode(s->Handle, &s->Mode)) {
    // ErrorExit("GetConsoleMode");
    return;
  }
  DWORD fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
  if (!SetConsoleMode(s->Handle, fdwMode)) {
    // ErrorExit("SetConsoleMode");
    return;
  }
}
void InitVirtualOutput(struct Stream *s) {
  s->Handle = GetStdHandle(STD_OUTPUT_HANDLE);
  if (!GetConsoleMode(s->Handle, &s->Mode)) {
    // ErrorExit("GetConsoleMode");
    return;
  }
  DWORD fdwMode = s->Mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  if (!SetConsoleMode(s->Handle, fdwMode)) {
    // ErrorExit("SetConsoleMode");
    return;
  }
}
void RestoreMode(struct Stream *s) { SetConsoleMode(s->Handle, s->Mode); }

int ReadInput(struct Stream *s, INPUT_RECORD *buf, DWORD buf_size,
              DWORD *numRead) {
  return ReadConsoleInput(s->Handle, buf, buf_size, numRead);
}

struct Stream g_stdin = {0};
struct Stream g_stdout = {0};

void term_init(void) {
  InitRawInput(&g_stdin);
  InitVirtualOutput(&g_stdout);
}

void term_done(void) {
  RestoreMode(&g_stdout);
  RestoreMode(&g_stdin);
}

void term_suspend(void) {}

INPUT_RECORD g_irInBuf[128];
DWORD g_cNumRead = 0;
size_t g_current = 0;

int term_read(void) {
  if (g_current >= g_cNumRead) {
    g_current = 0;
    if (!ReadInput(&g_stdin,       // input buffer handle
                   g_irInBuf,      // buffer to read into
                   128,            // size of read buffer
                   &g_cNumRead)) { // number of records read
      return -1;
      // ErrorExit("ReadConsoleInput");
    }
  }

  // Dispatch the events to the appropriate handler.
  for (; g_current < g_cNumRead;) {
    INPUT_RECORD record = g_irInBuf[g_current++];
    switch (record.EventType) {
    case KEY_EVENT: // keyboard input
      // KeyEventProc(irInBuf[i].Event.KeyEvent);
      {
        KEY_EVENT_RECORD e = record.Event.KeyEvent;
        if (e.bKeyDown) {
          return e.uChar.AsciiChar;
        }
      }
      break;

    case MOUSE_EVENT: // mouse input
      // MouseEventProc(irInBuf[i].Event.MouseEvent);
      break;

    case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
      // ResizeEventProc(irInBuf[i].Event.WindowBufferSizeEvent);
      break;

    case FOCUS_EVENT: // disregard focus events

    case MENU_EVENT: // disregard menu events
      break;

    default:
      // ErrorExit("Unknown event type");
      break;
    }
  }

  return -1;
}

int cmd_make(char **argv, int *ifd, int *ofd) { return -1; }

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
char *cmd_pipe(char *cmd, char *ibuf, int oproc) { return 0; }

void disableRawMode(int _) {}

// int getWindowSize(int ifd, int ofd, int *rows, int *cols) {
//
//   CONSOLE_SCREEN_BUFFER_INFO csbi;
//   if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi) ==
//   0) {
//     return -1;
//   }
//
//   *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
//   *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
//   return 0;
// }
