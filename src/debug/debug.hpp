/*
 * MIT License
 *
 * Copyright (c) 2022 Jiawei Wang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/


#define CHAINSAW_VERSION "0.6.0" 
#define DEBUG_COMPILE "g++ -std=c++17 -Wshadow -Wall -O2 -D DEBUG "
#define QUIT_TIMES 3

#define VS std::vector<std::string>

#define BUFFINIT {NULL, 0, false, 0}
#define BUFNULL 0
#define OUTBUF 1
#define OUTCMD 2
#define OUTLOAD 3
#define BUFOUT 4
#define DISPLAY 5

#define ERRFREE 0
#define COMPILE 1
#define RUNTIME 2

#define MACOSTIME "/usr/bin/time -l "
#define LINUXTIME "/usr/bin/time -v "

#define MACKEY "maximum resident"
#define MACREAL "real"

#define LINUXKEY "Average resident"
#define LINUXSHARE "Average shared"
#define LINUXBEG "Command being"

#include <termios.h>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <string.h>
#include <csignal>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <cstring>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <fcntl.h>
#include <set>
#include <cstdio>
#include <vector>
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

struct buffer {
  char *msg;
  size_t len;
  bool valid;
  int type;  /* 0 -> OUTNULL, 1 -> OUTBUF, 2 -> OUTCMD, 3 -> OUTLOAD */
  /* 4 -> BUFOUT, 5 -> DISPLAY */
};

class Editor {
public:
  /* Do not use std::string (hard to debug) */

  bool outMode;

  Editor(buffer* out, buffer* cmd, int* errFree, VS* runtime)
    : outbuffer(out), command(cmd), errFree(errFree), runtime(runtime) {};

  void RefreshScreen();

  void init() {
    Conf.cx = ConfOut.cx = 0;
    Conf.cy = ConfOut.cy = 0;
    Conf.offrow = ConfOut.offrow = 0;
    Conf.offcol = ConfOut.offcol = 0;
    Conf.numrows = ConfOut.numrows = 0;
    Conf.rows = ConfOut.rows = NULL;
    Conf.dirty = ConfOut.dirty = 0;
    Conf.filename = NULL;
    ConfOut.filename = NULL;
    Conf.command = "";
    Conf.commandst = false;
    outMode = false;
    this->display_buffer = BUFFINIT;
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

  int LoadOut(char *filename);

  void InsertRow(int at, char* line, size_t len, editorConfig& Conf);

  void UpdateRow(editRow* erow);

  void ProcessKeypress(int fd);

  int ReadKey(int fd);

  void SetStatusMessage(const char *fmt, ...);

  char* RowsToString(int *buflen, editorConfig& Conf);

  void MoveCursor(int key, editorConfig& Conf);

  void InsertChar(int c, editorConfig& Conf);

  void InsertCommand(int c, editorConfig& Conf);

  void Exit(int status);

protected:
  void displayAppend(const char* s, int len);
  void displayFree();
  void insertNewline(editorConfig& Conf);
  int save(editorConfig& Conf);
  void rowAppendString(editRow* erow, char *s, size_t len);
  void delChar();
  void delRow(int at);
  void rowDelChar(editRow* erow, int at);
  void rowInsertChar(editRow *row, int pos, int c, editorConfig& Conf);
  void commandPrompt();
  void transferCmd();
  void delCharCmd();
  void focusKeyPress(int c, editorConfig& Conf);


private:
  /* If editor is willing to send the msg to the outside, it will give a value to buffer */
  buffer* outbuffer;
  buffer* command;

  buffer display_buffer;


  int *errFree;

  VS* runtime;

  struct editorConfig Conf;
  struct editorConfig ConfOut; /* Config for the out (Compile error, debug output)*/

  void updateWindowSize() {
    if (getWindowSize(STDIN_FILENO, STDOUT_FILENO, &Conf.lmtrow, &Conf.lmtcol)) {
      perror("Unable to query the screen for size (ioctl failed))");
      Exit(1);
    }

    /*  -----------------------
     * |      editor (buf)     |
     *  -----------------------   <- lmtrow/2
     *  ~~~~~~~~~~~~~~~~~~~~~~~   <- 3 intermediate later
     *  -----------------------   <- lmtrow/2 - 3
     * |      outbuffer        |
     *  -----------------------
     */

    int tmpmax = Conf.lmtrow;
    ConfOut.lmtrow = Conf.lmtrow /= 2;
    ConfOut.lmtcol = Conf.lmtcol;
    ConfOut.lmtrow -= (tmpmax % 2) ? 3 : 4; // Get room for status bar.
  };

  void handleSigWinCh() { 
    updateWindowSize();
    if (Conf.cy > Conf.lmtrow) Conf.cy = Conf.lmtrow - 1;
    if (ConfOut.cy > ConfOut.lmtrow) ConfOut.cy = ConfOut.lmtrow - 1;
    if (Conf.cx > Conf.lmtcol) Conf.cx = Conf.lmtcol - 1;
    if (ConfOut.cx > ConfOut.lmtcol) ConfOut.cx = ConfOut.lmtcol - 1;
    RefreshScreen();
  }

  // Try to get the number of columns in the current terminal. If the ioctl()
  // call fails the function will try to query the terminal itself. 
  // Returns 0 on success, -1 on error
  int getWindowSize(int ifd, int ofd, int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
      /* if ioctl() failed, then we just failed TODO: handle this case */
      return -1;
    } else {
      *cols = ws.ws_col;
      *rows = ws.ws_row;
      return 0;
    }
  };

  void set_cursor(editorConfig& Conf, char* buf) {
    /* Finally, put cursor at its current position. */
    int j;
    int cx = 1;
    int filerow = Conf.offrow + Conf.cy;
    editRow *row = (filerow >= Conf.numrows) ? NULL : &Conf.rows[filerow];

    if (row) {
      for (j = Conf.offcol; j < (Conf.cx + Conf.offcol); j++) {
	if (j < row->size && row->chars[j] == TAB) cx += 7 - ((cx) % 8);
	cx++;
      }
    }

    int buflen;

    if (&Conf == &this->ConfOut) {
      buflen = snprintf(buf, sizeof(buf), "\033[%d;%dH", Conf.cy + this->Conf.lmtrow + 4, cx); // set the position of the cursor
      // SetStatusMessage("Cursor Out: %d, %d, offrow: %d", Conf.cy + this->Conf.lmtrow + 3, cx, Conf.offrow);
    } else { 
      buflen = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", Conf.cy+1, cx); // set the position of the cursor
      // SetStatusMessage("Cursor: %d, %d, offrow: %d", Conf.cy, cx, Conf.offrow);
    }

    displayAppend(buf, buflen);
    displayAppend("\x1b[?25h", 6);
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
  void init() {
    this->infiles = {};
  };

  Inter(std::string file, buffer* out, buffer* command, buffer* outload, buffer* bufout, int* errFree, VS* runtime) 
    : execfile(file), outbuffer(out), command(command), outload(outload),  bufout(bufout), errFree(errFree), runtime(runtime) {};

  bool Exec(char* cmd);

  void RemoveTmp();

protected:
  bool infile(std::string filename);
  bool outexe(std::string filename);

  void parseMac();
  void parseLinux();

  std::string shellExe(const char* cmd);
  int shellExec(const char* cmd);

private:
  std::string execfile;
  int exit_code_;
  std::string stdout_file_;
  std::string stderr_file_;
  std::string tmp_dir_;
  int *errFree;
  VS* runtime;

  /* Change all of the ipc msgs into a pointer buffer */
  buffer* command;
  buffer* outbuffer;
  buffer* outload;
  buffer* bufout;
  int* errType;

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

  void set_buffer(std::string bufferstr) {
    this->outbuffer->msg = (char *) std::malloc(sizeof(bufferstr.c_str()));
    std::strcpy(this->outbuffer->msg, bufferstr.c_str());
    this->outbuffer->len = sizeof(bufferstr.c_str());
    this->outbuffer->valid = true;
    this->outbuffer->type = OUTBUF;
  };

  std::string file2str(std::string filename) {
    std::ifstream file;
    std::stringstream str_stream;

    file.open(filename, std::ios_base::app);
    if (file.is_open()) {
      str_stream << file.rdbuf();
      file.close();
    }
    return str_stream.str();
  };
};
