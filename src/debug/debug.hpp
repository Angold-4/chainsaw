#define CHAINSAW_VERSION "0.3.0"

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <iostream>
#include <time.h>

enum KEY_ACTION{
  KEY_NULL = 0,       /* NULL */
  CTRL_C = 3,         /* Ctrl-c */
  CTRL_D = 4,         /* Ctrl-d */
  CTRL_F = 6,         /* Ctrl-f */
  CTRL_H = 8,         /* Ctrl-h */
  TAB = 9,            /* Tab */
  CTRL_L = 12,        /* Ctrl+l */
  ENTER = 13,         /* Enter */
  CTRL_Q = 17,        /* Ctrl-q */
  CTRL_S = 19,        /* Ctrl-s */
  CTRL_U = 21,        /* Ctrl-u */
  ESC = 27,           /* Escape */
  BACKSPACE =  127,   /* Backspace */
  /* The following are just soft codes, not really reported by the
   * terminal directly. */
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};


/* We define a very simple "append buffer" structure, that is an heap
 * allocated string where we can append to. This is useful in order to
 * write all the escape sequences in a buffer and flush them to the standard
 * output in a single call, to avoid flickering effects. */
struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL,0}

void abAppend(struct abuf *ab, const char *s, int len);

// This structure represents a single line of the file we are editing
struct editRow {
  int idx;            // Row index in the file
  int size;           // Size of the row
  int rsize;          // Size of the rendered row
  char *chars;        // Raw content of the row
  char *render;       // Row content "rendered" for screen display (for TABs)
};

struct editorConfig {
  int cx, cy;         // Cursor position
  int offrow, offcol; // Offset of rows and cols displayed
  int lmtrow, lmtcol; // # of rows and cols we can show
  int numrows;        // Total # of rows in this file
  int rawmode;        // Is terminal raw mode enabled?
  editRow *rows;      // Rows of this file
  int dirty;          // File modified but not saved
  char *filename;     // Currently open filename
  char statusmsg[80]; // Msg want to show to the user
  time_t statusmsg_time;
};


class Editor {
public:
  void RefreshScreen();

  void init() {
    Conf.cx = Conf.cy = 0;
    Conf.offrow = Conf.offrow = 0;
    Conf.numrows = 0;
    Conf.rows = NULL;
    Conf.dirty = 0;
    Conf.filename = NULL;
    updateWindowSize();
  };

private:
  struct editorConfig Conf;

  void updateWindowSize() {
    if (getWindowSize(STDIN_FILENO, STDOUT_FILENO, &Conf.lmtrow, &Conf.lmtcol)) {
      perror("Unable to query the scrren for size (ioctl failed))");
      exit(1);
    }
    Conf.lmtrow /= 2; // Get room for the interpreter (inter)
    Conf.lmtrow -= 2; // Get room for status bar.
  };

  void handleSigWinCh(int unused __attribute__((unused))) { updateWindowSize();
    if (Conf.cy > Conf.lmtrow) Conf.cy = Conf.lmtrow - 1;
    if (Conf.cx > Conf.lmtcol) Conf.cx = Conf.lmtcol - 1;
    RefreshScreen();
  }

  // Try to get the number of columns in the current terminal. If the ioctl()
  // call fails the function will try to query the terminal itself. 
  // Returns 0 on success, -1 on error
  int getWindowSize(int ifd, int ofd, int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
      /* if ioctl() failed, then we just failed TODO: handle this case*/
      return -1;
    } else {
      *cols = ws.ws_col;
      *rows = ws.ws_row;
      return 0;
    }
  }
};
