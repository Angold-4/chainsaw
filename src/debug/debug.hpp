#define CHAINSAW_VERSION "0.3.0"
#define DEBUG_COMPILE "g++ -std=c++17 -Wshadow -Wall -O2 -D DEBUG "
#define QUIT_TIMES 3

#include <termios.h>
#include <cstdlib>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <cstring>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <fcntl.h>
#include <set>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

enum KEY_ACTION{
  KEY_NULL = 0,       /* NULL */
  CTRL_C = 3,         /* Ctrl-c for command */
  CTRL_D = 4,         /* Ctrl-d for edit */
  CTRL_F = 6,         /* Ctrl-f */
  CTRL_H = 8,         /* Ctrl-h */
  TAB = 9,            /* Tab (Ctrl-i) */
  CTRL_J = 10,        /* Ctrl-j */
  CTRL_K = 11,        /* Ctrl-k */
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

static struct termios orig_termios; /* In order to restore at exit.*/

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
  int cmdrow, cmdcol; // # of rows and cols for the command area
  int numrows;        // Total # of rows in this file
  int rawmode;        // Is terminal raw mode enabled?
  editRow *rows;      // Rows of this file
  int dirty;          // File modified but not saved
  char *filename;     // Currently open filename
  char statusmsg[80]; // Msg want to show to the user
  std::string command;   // User input Command-line msg
  bool commandst;        // Whether we are in the command status
  time_t statusmsg_time;
};


class Editor {
public:
  std::string outbuffer;
  std::string command;

  void RefreshScreen();

  void init() {
    outbuffer = "";
    Conf.cx = 0;
    Conf.cy = 0;
    Conf.offrow = Conf.offrow = 0;
    Conf.numrows = 0;
    Conf.rows = NULL;
    Conf.dirty = 0;
    Conf.filename = NULL;
    Conf.command = "";
    Conf.commandst = false;
    updateWindowSize();
  };

  /* Raw mode: 1960 magic shit. */
  int EnableRawMode(int fd) {
    struct termios raw;

    if (Conf.rawmode) return 0; /* Already enabled. */
    if (!isatty(STDIN_FILENO)) goto fatal;
    if (tcgetattr(fd,&orig_termios) == -1) goto fatal;

    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer. */
    raw.c_cc[VMIN] = 0; /* Return each byte, or zero for timeout. */
    raw.c_cc[VTIME] = 1; /* 100 ms timeout (unit is tens of second). */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
    Conf.rawmode = 1;
    return 0;

  fatal:
    errno = ENOTTY;
    return -1;
  }

  int Open(char *filename);

  void InsertRow(int at, char* line, size_t len);

  void UpdateRow(editRow* erow);

  void ProcessKeypress(int fd);

  int ReadKey(int fd);

  void SetStatusMessage(const char *fmt, ...);

  char* RowsToString(int *buflen);

  void MoveCursor(int key);

  void InsertChar(int c);

  void InsertCommand(int c);

protected:
  void insertNewline();
  int save();
  void rowAppendString(editRow* erow, char *s, size_t len);
  void delChar();
  void delRow(int at);
  void rowDelChar(editRow* erow, int at);
  void rowInsertChar(editRow *row, int pos, int c);
  void commandPrompt();

private:
  struct editorConfig Conf;

  void updateWindowSize() {
    if (getWindowSize(STDIN_FILENO, STDOUT_FILENO, &Conf.lmtrow, &Conf.lmtcol)) {
      perror("Unable to query the scrren for size (ioctl failed))");
      std::system("clear");
      exit(1);
    }
    Conf.cmdrow = Conf.lmtrow/2 - 1;
    Conf.lmtrow /= 2; // Get room for the interpreter (inter)
    Conf.lmtrow -= 3; // Get room for status bar.
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
  };

  /* Free row's heap allocated stuff. */
  void freeRow(editRow *row) {
    std::free(row->render);
    std::free(row->chars);
  }
};


/* Interpreter: used for processing user command, passed from editor 
 * and give the output to the editor. */
class Inter {
public:
  bool error;
  std::string infilename;
  std::string command;
  std::string outbuffer;

  Inter(std::string file) : execfile(file) {};

  void init() {
    error = false;
    infilename = "";
    command = "";
    outbuffer = "";
  };

  void Exec(std::string cmd);

protected:
  void infile(std::string filename);
  void outexe(std::string filename);

  std::string shellExe(const char* cmd);

private:
  std::set<std::string> infiles;

  std::string getFileName(std::string cmd) {
    std::string filename;
    if (cmd[0] != ' ') return "";
    int i = 1;
    while (i != cmd.size() && cmd[i] != ' ') {
      filename += cmd[i];
      i++;
    }
    return filename;
  };

  std::string remove_suffix(std::string str) {
    std::string ret = "";
    int i = 0;
    while (str[i] != '.' || i != str.size()) {
      ret += str[i];
      i++;
    }
    return ret;
  };

  std::string execfile;
};
