#include "debug.hpp"

void Editor::RefreshScreen() {
  int y;
  editRow *r;
  char buf[32];
  std::string buffer = "";

  buffer += "\x1b[?25l"; // Hide cursor
  buffer += "\x1b[H";    // Go home

  for (y = 0; y < Conf.lmtrow; y++) { // 0
    int filerow = Conf.offrow + y;
    if (filerow >= Conf.numrows) { // init case (first time call)
      if (Conf.numrows == 0 && y == Conf.lmtrow/3) { // (print welcome msg in the 1/3 pos)
	char welcome[80];
	int welcomelen = snprintf(welcome, sizeof(welcome),
	    "Chainsaw debug editor -- version %s\x1b[0K\r\n]", CHAINSAW_VERSION);
	int padding = (Conf.lmtcol-welcomelen)/2;
	if (padding) {
	  buffer += "~";
	  padding--;
	}
	while (padding--) buffer += " ";
	std::string swelcome(welcome);
	// abAppend(&ab, welcome, welcomelen);
	buffer += swelcome;
      } else {
	buffer += "~\x1b[0K\r\n"; // ~ (empty)
      }
      continue;
    }

    r = &Conf.rows[filerow];

    int len = r->rsize - Conf.offcol; // offset

    if (len > 0) {
      if (len > Conf.lmtcol) len = Conf.lmtcol;
      char *c = r->render + Conf.offcol; // start point
      int j;
      for (j = 0; j < len; j++) {
	buffer += *(c+j);
      }
    }

    buffer += "\x1b[39m";
    buffer += "\x1b[0K";
    buffer += "\r\n";
  }


  /* Create a two row status. The first row */
  buffer += "\x1b[0K";
  buffer += "\x1b[7m";


  char status[80], rstatus[80];

  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
      Conf.filename, Conf.numrows, Conf.dirty ? "(modified)" : "");

  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
      Conf.offrow + Conf.cy + 1, Conf.numrows);

  if (len > Conf.lmtcol) len = Conf.lmtcol;

  std::string ss(status); std::string rs(rstatus);
  buffer += ss;

  while (len < Conf.lmtcol) {
    if (Conf.lmtcol - len == rlen) {
      buffer += rs;
      break;
    } else {
      buffer += " ";
      len++;
    }
  }

  buffer += "\x1b[0m\r\n";

  /* Second row depends on Conf.statusmsg and the status message update time */
  buffer += "\x1b[0K";
  int msglen = strlen(Conf.statusmsg);
  if (msglen && time(NULL) - Conf.statusmsg_time < 5) {
    // abAppend(&ab, Conf.statusmsg, msglen <= Conf.lmtcol ? msglen : Conf.lmtcol);
    std::string sstatusmsg(Conf.statusmsg);
    buffer += sstatusmsg;
  }

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

  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", Conf.cy+1, cx);
  std::string sbuf(buf);
  buffer += sbuf;
  buffer += "\x1b[?25h"; // show the cursor
  std::cout << buffer << std::endl;
}

/* Load the specified program in the editor memory and returns 0 on success */
int Editor::Open(char* filename) {
  FILE* fp;
  Conf.dirty = 0;
  size_t fnlen = strlen(filename) + 1;
  Conf.filename = new char[fnlen];
  std::memcpy(Conf.filename, filename, fnlen);

  fp = fopen(filename, "r");
  if (!fp) {
    if (errno != ENOENT) {
      perror("Opening file");
      exit(1);
    }
    return 1;
  }

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    /* line by line, when meet newline (\n or \r), replace it with a terminal */
    if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r')) {
      line[--linelen] = '\0';
    }
    InsertRow(Conf.numrows, line, linelen);
  }
  return 0;
};

/* Insert a row at the specified position, shifting the other rows on the bottom */
void Editor::InsertRow(int pos, char *line, size_t len) {
  if (pos > Conf.numrows) return;

  /* reallocate rows in the heap */
  Conf.rows = (editRow *) realloc(Conf.rows, sizeof(editRow) * (Conf.numrows+1));

  if (pos != Conf.numrows) { // Do not append to the end 
    /* Insert the row into middle, shifting other rows on the bottom */
    memmove(Conf.rows+pos+1, Conf.rows+pos, sizeof(Conf.rows[0]) * (Conf.numrows-pos));
    for (int j = pos+1; j <= Conf.numrows; j++) Conf.rows[j].idx++; // update the index
  }

  Conf.rows[pos].size = len;
  Conf.rows[pos].chars = new char[len];
  std::memcpy(Conf.rows[pos].chars, line, len+1);
  Conf.rows[pos].render = NULL; // not rendered yet
  Conf.rows[pos].rsize = 0;
  Conf.rows[pos].idx = pos;

  UpdateRow(Conf.rows + pos);

  Conf.numrows++;
  Conf.dirty++;
};


/* Update the rendered version of a row (print to screen) */
void Editor::UpdateRow(editRow* erow) {
  unsigned int tabs = 0, nonprint = 0;
  int j, idx;

  /* Create a version of the row we can directly print on the screen,
   * respecting tabs, substituting non-printable characters with '?' */

  delete[] erow->render;
  for (j = 0; j < erow->size; j++) { // for each char
    if (erow->chars[j] == TAB) tabs++;
  }
  
  unsigned long long allocsize =
    (unsigned long long) erow->size + tabs*8 + nonprint*9 + 1;

  if (allocsize > UINT32_MAX) {
    printf("Some line of the edited debug input is too long\n");
    exit(1);
  }

  erow->render = new char[allocsize];
  idx = 0;
  for (j = 0; j < erow->size; j++) {
    if (erow->chars[j] == TAB) {
      erow->render[idx++] = ' ';
      while ((idx+1) % 8 != 0) erow->render[idx++] = ' ';
    } else {
      erow->render[idx++] = erow->chars[j];
    }
  }
  erow->rsize = idx;
  erow->render[idx] = '\0';
};

/* Set an editor status message for the second line of the status, at the
 * end of the screen. */
void Editor::SetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap,fmt);
  vsnprintf(Conf.statusmsg,sizeof(Conf.statusmsg),fmt,ap);
  va_end(ap);
  Conf.statusmsg_time = time(NULL);
};

/* Process events arriving from the standard input */
void Editor::ProcessKeypress(int fd) {
  /* When the file is modified, requires Ctrl-q to be pressed N times before quitting */
  static int quit_times = QUIT_TIMES;

  int c = ReadKey(fd);

  switch(c) {
    case ENTER:     /* Enter */
      this->insertNewline();
      break;
    case CTRL_C:    /* Ctrl-c */
      /* We just ignore ctrl-c, since it can be so simple to lose the changes */
      break;
    case CTRL_Q:    /* Ctrl-q */
      /* Quit if the file was already saved, or we want it to quit directly */
      if (Conf.dirty && quit_times) {
	SetStatusMessage("WARNING!!! FILE has unsaved changes." 
	    "Press Ctrl-Q %d more times to quit.", quit_times);
	quit_times--;
      }
      exit(0);
      break;
    case CTRL_S:    /* Ctrl-s */
      save();
      break;
    case BACKSPACE:
    case CTRL_H:
    case DEL_KEY:
      delChar();
      break;
  }

  quit_times = QUIT_TIMES; // reset it to the original value
};

int Editor::ReadKey(int fd) {
  int nread;
  char c, sq[3];
  while ((nread = read(fd, &c, 1)) == 0);
  if (nread == -1) exit(1);
  while (1) {
    switch(c) {
      /* TODO: Handle ESC [ and ESC O sequences */
      default:
	return c;
    }
  }
};

char* Editor::RowsToString(int *buflen) {
  char *buf = NULL, *p; // pointer
  int totlen = 0;
  int j;

  /* Compute count of bytes */
  for (j = 0; j < Conf.numrows; j++) {
    totlen += Conf.rows[j].size + 1; // +1 is the for the '\n'
  }
  *buflen = totlen;
  totlen++; // also make space for nulterm

  p = buf = (char *) std::malloc(totlen);
  for (j = 0; j < Conf.numrows; j++) {
    std::memcpy(p, Conf.rows[j].chars, Conf.rows[j].size);
    p += Conf.rows[j].size;
    *p = '\n';
    p++;
  }
  
  *p = '\0';
  return buf; // buf is the start of the buffer
}

/* Performing keypress actions */

/* Inserting a newline is slightly complex as we have to handling inserting a 
 * newline in the middle of a line */
void Editor::insertNewline() {
  int filerow = Conf.offrow + Conf.cy;
  int filecol = Conf.offcol + Conf.cx;
  editRow *row = (filerow >= Conf.numrows) ? NULL : &Conf.rows[filerow];
  
  if (!row) {
    /* Append newline in the end */
    if (filerow == Conf.numrows) {
      InsertRow(filerow, strdup(""), 0);
      goto fixcursor;
    }
    return;
  }

  if (filecol >= row->size) filecol = row->size;
  if (filecol == 0) {
    /* If the cursor is over the current line size, we want to conceptually
     * think it's just over the last character */
    InsertRow(filerow, strdup(""), 0);
  } else {
    /* We are in the middle of a line, Split it between two rows. */
    InsertRow(filerow+1, row->chars+filecol, row->size-filecol);
    row = &Conf.rows[filerow];
    row->chars[filecol] = '\0';
    row->size = filecol;
    UpdateRow(row); // update the rendered version
  }

fixcursor:
  if (Conf.cy == Conf.lmtrow-1) {
    Conf.offrow++; // the whole display offset++;
  } else {
    Conf.cy++;     // or just move the cursor
  }
  Conf.cx = 0;
  Conf.offcol = 0;
};

/* Save the current file on disk. Return 0 on success. */
int Editor::save() {
  int len;
  char* buf = RowsToString(&len);
  int fd = open(Conf.filename, O_RDWR|O_CREAT, 0644);
  if (fd == -1) goto writeerr;

  /* Use truncate + a single write call in order to make saving  a bit safer */
  if (ftruncate(fd, len) == -1) goto writeerr;
  if (write(fd, buf, len) != len) goto writeerr;

  close(fd);
  std::free(buf);
  Conf.dirty = 0;
  SetStatusMessage("%d bytes written on disk", len);
  return 0;

writeerr:
  std::free(buf);
  if (fd != -1) close(fd);
  SetStatusMessage("Can't save! I/O error: %s", strerror(errno));
  return 1;
};

/* Append the string 's' at the end of a row */
void Editor::rowAppendString(editRow* row, char *s, size_t len) {
  row->chars = (char *) std::realloc(row->chars, row->size+len+1);
  std::memcpy(row->chars + row->size, s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  UpdateRow(row);
  Conf.dirty++;
};

/* Delete the char at the current prompt position */
void Editor::delChar() {
  int filerow = Conf.offrow + Conf.cy;
  int filecol = Conf.offcol + Conf.cx;
  editRow *row = (filerow >= Conf.numrows) ? NULL : &Conf.rows[filerow];

  if (!row || (filerow == 0 && filecol == 0)) return;

  if (filecol == 0) {
    /* Handle the case of column 0, we need to move the current line
     * on the right of the previous one. */
    filecol = Conf.rows[filerow-1].size;
    rowAppendString(&Conf.rows[filerow-1], row->chars, row->size); // combine two lines
    delRow(filerow);
    row = NULL;
    if (Conf.cy == 0)
      Conf.offrow--;
    else
      Conf.cy--;
    Conf.cx = filecol;

    if (Conf.cx >= Conf.lmtcol) {
      int shift = (Conf.lmtcol - Conf.cx) + 1;
      Conf.cx -= shift;
      Conf.offcol += shift;
    }
  } else {
    /* Just normal delete */
    rowDelChar(row, filecol-1);
    if (Conf.cx == 0 && Conf.offcol) {
      Conf.offcol--;
    } else {
      Conf.cx--;
    }
  }
  if (row) UpdateRow(row);
  Conf.dirty++;
};

/* Remove the row at the specific position, shifting the remaining of the top */
void Editor::delRow(int at) {
  editRow* row;

  if (at >= Conf.numrows) return;
  row = Conf.rows + at;
  freeRow(row);
  /* memove(dst, src, len) shift the remaining */
  std::memmove(Conf.rows+at, Conf.rows+at+1, sizeof(Conf.rows[0]) * (Conf.numrows-at-1));
  for (int j = at; j < Conf.numrows-1; j++) Conf.rows[j].idx++;
  Conf.numrows--;
  Conf.dirty++;
};

/* Delete the character at offset 'at' from the specified row. */
void Editor::rowDelChar(editRow* row, int pos) {
  if (row->size <= pos) return;
  std::memmove(row->chars+pos, row->chars+pos+1, row->size-pos);
  UpdateRow(row);
  row->size--;
  Conf.dirty++;
};
