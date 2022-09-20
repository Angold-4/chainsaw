#include "debug.hpp"

void Editor::RefreshScreen() {
  int y;
  editRow *r;
  char buf[32];

  std::string buffer = ""; /* Dynamic append to the buffer */

  buffer += "\x1b[?25l"; // Hide cursor
  buffer += "\x1b[H";    // Go home

  for (y = 0; y < Conf.lmtrow; y++) { // 0

    /* offrow means the current row offset of file */

    int filerow = Conf.offrow + y; // the current row(line) in the file

    if (filerow >= Conf.numrows) {
      if (Conf.numrows == 0 && y == Conf.lmtrow/2) { // (print welcome msg in the 1/2 pos)
	char welcome[80];
	int welcomelen = snprintf(welcome, sizeof(welcome),
	    "Chainsaw debug editor -- version %s\x1b[0K\r\n", CHAINSAW_VERSION);
	int padding = (Conf.lmtcol-welcomelen)/2;
	if (padding) {
	  buffer += "~";
	  padding--;
	}
	while (padding--) buffer += " ";
	std::string swelcome(welcome);
	buffer += swelcome;
      } else {
	buffer += "~\x1b[0K\r\n"; // ~ (empty)
      }
      continue;
    }

    /* if we already have some rows (numrows > 0) */

    r = &Conf.rows[filerow];

    int len = r->rsize - Conf.offcol; // offset

    if (len > 0) { /* if offset is greater than rsize, then we should print nothing */
      if (len > Conf.lmtcol) len = Conf.lmtcol; // fix it
      char *c = r->render + Conf.offcol;
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
      Conf.offrow + Conf.cy, Conf.numrows);

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

  buffer += "\x1b[0K";
  /* Second row depends on Conf.statusmsg and the status message update time */
  int msglen = strlen(Conf.statusmsg);

  if (Conf.commandst) {
    buffer += Conf.command;
  } else if (msglen && time(NULL) - Conf.statusmsg_time < 2) {
    std::string sstatusmsg(Conf.statusmsg);
    buffer += sstatusmsg;
  }
  buffer += "\x1b[0m\r\n";

  buffer += "\x1b[0K";

  buffer += "\x1b[7m";

  /* TODO: change empty row into running status (speed, memory usage) */

  std::string emptyrow(Conf.lmtcol,' ');
  buffer += emptyrow;
  buffer += "\x1b[0m\r\n";

  for (y = 0; y < ConfOut.lmtrow; y++) { // 0
    int filerow = ConfOut.offrow + y;

    if (filerow >= ConfOut.numrows) {
      if (ConfOut.numrows == 0 && y == ConfOut.lmtrow/2) { // (print buffer msg in the 1/2 pos)
	char welcome[80];
	int welcomelen = snprintf(welcome, sizeof(welcome),
	    "Debug Output Buffer");
	int padding = (ConfOut.lmtcol-welcomelen)/2;
	if (padding) {
	  buffer += "";
	  padding--;
	}
	while (padding--) buffer += " ";
	std::string swelcome(welcome);
	buffer += swelcome;
      } else {
	buffer += "\x1b[0K\r\n"; //  (empty)
      }
      continue;
    }

    r = &ConfOut.rows[filerow];

    int len = r->rsize - ConfOut.offcol; // offset

    if (len > 0) {
      if (len > ConfOut.lmtcol) len = ConfOut.lmtcol;
      char *c = r->render + ConfOut.offcol; // start point
      int j;
      for (j = 0; j < len; j++) {
	buffer += *(c+j);
      }
    }

    buffer += "\x1b[39m";
    buffer += "\x1b[0K";
    buffer += "\r\n";
  }

  if (this->outMode) {
    set_cursor(ConfOut, buf, buffer);
  } else {
    set_cursor(Conf, buf, buffer);
  }

  write(STDOUT_FILENO, buffer.c_str(), buffer.size()+1);

  buffer.clear();
}

/* Load the specified program in the editor memory and returns 0 on success */
int Editor::Open(char* filename) {
  FILE* fp;
  Conf.dirty = 0;
  size_t fnlen = strlen(filename) + 1;
  Conf.filename = (char*) std::malloc(fnlen);
  std::memcpy(Conf.filename, filename, fnlen);

  fp = fopen(filename, "r");
  if (!fp) {
    if (errno != ENOENT) {
      perror("Opening file");
      Exit(1);
    }
    return 1;
  }

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  /* Since we're going to clean the row status, 
   * we do not want if there are any unsaved changes */
  if (Conf.dirty) {
    /* Just Save it automatically */
    SetStatusMessage("Automatically saved file: %s", Conf.filename);
    save(Conf);
  }

  /* init the cursor */
  delete Conf.rows;
  Conf.rows = nullptr;
  Conf.numrows = 0;
  Conf.cx = Conf.cy = 0;

  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    /* line by line, when meet newline (\n or \r), replace it with a terminal */
    if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r')) {
      line[--linelen] = '\0';
    }
    InsertRow(Conf.numrows, line, linelen, Conf);
  }
  return 0;
};


/* Load the out buffer (debug output, compile err) into editor */
int Editor::LoadOut(char* filename) {
  FILE* fp;
  ConfOut.dirty = 0;
  size_t fnlen = strlen(filename) + 1;
  ConfOut.filename = new char[fnlen];
  std::memcpy(ConfOut.filename, filename, fnlen);

  fp = fopen(filename, "r");
  if (!fp) {
    if (errno != ENOENT) {
      perror("Opening file");
      Exit(1);
    }
    return 1;
  }

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  /* init the cursor */
  delete ConfOut.rows;
  ConfOut.rows = nullptr;
  ConfOut.numrows = 0;
  ConfOut.cx = ConfOut.cy = 0;

  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    /* line by line, when meet newline (\n or \r), replace it with a terminal */
    if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r')) {
      line[--linelen] = '\0';
    }
    InsertRow(ConfOut.numrows, line, linelen, ConfOut);
  }

  return 0;
};


/* Insert a row at the specified position, shifting the other rows on the bottom */
void Editor::InsertRow(int pos, char *line, size_t len, editorConfig& Conf) {
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
  // Conf.dirty++;
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
    Exit(1);
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
  int c = ReadKey(fd);
  if (outMode) {
    /* Cursor is in Out Buffer */
    focusKeyPress(c, this->ConfOut);
  } else {
    focusKeyPress(c, this->Conf);
  }
};

/* Process events arriving from the standard input according to which buffer we are in */
void Editor::focusKeyPress(int c, editorConfig& Conf) {
  /* When the file is modified, requires Ctrl-q to be pressed N times before quitting */
  static int quit_times = QUIT_TIMES;

  switch(c) {
    case ENTER:     /* Enter or Exec */
      if (Conf.commandst) { /* Transfer the cmd msg to outside */
	transferCmd();
      } else if (!outMode) {
	insertNewline(Conf);
      }
      break;
    case CTRL_C:    /* Ctrl-c */
      /* Enter / Quit the command mode */
      Conf.commandst = !Conf.commandst;
      Conf.command = ":";
      break;
    case CTRL_D:    /* Ctrl-d */
      /* Switch between buffer */
      this->outMode = !this->outMode;
      break;
    case CTRL_Q:    /* Ctrl-q */
      /* Quit if the file was already saved, or we want it to quit directly */
      if (this->Conf.dirty && quit_times) {
	SetStatusMessage("WARNING!!! FILE has unsaved changes." 
	    "Press Ctrl-Q %d more times to quit.", quit_times);
	quit_times--;
	return;
      }
      Exit(0);
      break;
    case CTRL_S:    /* Ctrl-s */
      if (!outMode) {
	save(Conf);
      }
      break;
    case BACKSPACE:
    case DEL_KEY:
      if (Conf.commandst) {
	delCharCmd();
      } else if (!outMode) {
	delChar();
      }
      break;
    case ARROW_UP:
    case CTRL_J:
    case ARROW_DOWN:
    case CTRL_K:
    case ARROW_LEFT:
    case CTRL_H:
    case ARROW_RIGHT:
    case CTRL_L:
      MoveCursor(c, Conf);
      break;
    default:
      if (Conf.commandst) InsertCommand(c, Conf);
      else if (!outMode) InsertChar(c, Conf);
      break;
  }
  quit_times = QUIT_TIMES; // reset it to the original value

};

int Editor::ReadKey(int fd) {
  int nread;
  char c, seq[3];
  while ((nread = read(fd, &c, 1)) == 0);
  if (nread == -1) Exit(1);
  while (1) {
    switch(c) {
      case ESC:    /* escape sequence */
	/* If this is just an ESC, we'll timeout here. */
	if (read(fd,seq,1) == 0) return ESC;
	if (read(fd,seq+1,1) == 0) return ESC;

	/* ESC [ sequences. */
	if (seq[0] == '[') {
	  if (seq[1] >= '0' && seq[1] <= '9') {
	    /* Extended escape, read additional byte. */
	    if (read(fd,seq+2,1) == 0) return ESC;
	    if (seq[2] == '~') {
		switch(seq[1]) {
		case '3': return DEL_KEY;
		case '5': return PAGE_UP;
		case '6': return PAGE_DOWN;
		}
	      }
	  } else {
	    switch(seq[1]) {
	    case 'A': return ARROW_UP;
	    case 'B': return ARROW_DOWN;
	    case 'C': return ARROW_RIGHT;
	    case 'D': return ARROW_LEFT;
	    case 'H': return HOME_KEY;
	    case 'F': return END_KEY;
	    }
	  }
	}

	/* ESC O sequences. */
	else if (seq[0] == 'O') {
	  switch(seq[1]) {
	  case 'H': return HOME_KEY;
	  case 'F': return END_KEY;
	  }
	}
	break;
      default:
	return c;
    }
  }
};

char* Editor::RowsToString(int *buflen, editorConfig& Conf) {
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
};


/* Performing keypress actions */

/* Insert a character at specific position in a row, moving the 
 * remaining chars on the right if needed */
void Editor::rowInsertChar(editRow *row, int pos, int c, editorConfig& Conf) {
  if (pos > row->size) {
    int padlen = pos-row->size;
    row->chars = (char*) std::realloc(row->chars, row->size+padlen+2);
    memset(row->chars+row->size, ' ', padlen); // write padlen ' ' into chars end
    row->chars[row->size+padlen+1] = '\0';
    row->size += padlen+1;
  } else {
    row->chars = (char*) std::realloc(row->chars, row->size+2); 
    std::memmove(row->chars+pos+1, row->chars+pos, row->size-pos+1);
    row->size++;
  }
  row->chars[pos] = c;
  UpdateRow(row);
  Conf.dirty++;
};

/* Insert the specified char at the current prompt position */
void Editor::InsertChar(int c, editorConfig& Conf) {
  int filerow = Conf.offrow + Conf.cy;
  int filecol = Conf.offcol + Conf.cx;
  editRow *row = (filerow >= Conf.numrows) ? NULL : &Conf.rows[filerow];

  if (!row) {
    while (Conf.numrows <= filerow) {
      InsertRow(Conf.numrows, strdup(""), 0, Conf);
    }
  }

  row = &Conf.rows[filerow];
  rowInsertChar(row, filecol, c, Conf);

  if (Conf.cx == Conf.lmtcol-1) {
    Conf.offcol++;
  } else {
    Conf.cx++;
  }
  Conf.dirty++;
};

/* Inserting a newline is slightly complex as we have to handling inserting a 
 * newline at the middle of a line */
void Editor::insertNewline(editorConfig& Conf) {
  int filerow = Conf.offrow + Conf.cy;
  int filecol = Conf.offcol + Conf.cx;
  editRow *row = (filerow >= Conf.numrows) ? NULL : &Conf.rows[filerow];
  
  if (!row) {
    /* Append newline in the end */
    if (filerow == Conf.numrows) {
      InsertRow(filerow, strdup(""), 0, Conf);
      goto fixcursor;
    }
    return;
  }

  if (filecol >= row->size) filecol = row->size;
  if (filecol == 0) {
    /* If the cursor is over the current line size, we want to conceptually
     * think it's just over the last character */
    InsertRow(filerow, strdup(""), 0, Conf);
  } else {
    /* We are in the middle of a line, Split it between two rows. */
    InsertRow(filerow+1, row->chars+filecol, row->size-filecol, Conf);
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
int Editor::save(editorConfig& Conf) {
  int len;
  char* buf = RowsToString(&len, Conf);
  int fd = open(Conf.filename, O_RDWR|O_CREAT, 0644);
  if (fd == -1) goto writeerr;

  /* Use truncate + a single write call in order to make saving a bit safer */
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
  SetStatusMessage("Can't save! I/O error: %s, %s", strerror(errno), Conf.filename);
  return 1;
};

/* Handle cursor position change because arrow keys were pressed. */
void Editor::MoveCursor(int key, editorConfig& Conf) {
  int filerow = Conf.offrow + Conf.cy;
  int filecol = Conf.offcol + Conf.cx;
  int rowlen;
  editRow *row = (filerow >= Conf.numrows) ? NULL : &Conf.rows[filerow];

  switch(key) {
    case ARROW_LEFT:
    case CTRL_H:
      if (Conf.cx == 0) {  // end of the line
	if (Conf.offcol) {
	  Conf.offcol--;
	} else {
	  if (filerow > 0) {
	    Conf.cy--;
	    Conf.cx = Conf.rows[filerow-1].size; // last element of last row
	    if (Conf.cx > Conf.lmtcol-1) {
	      Conf.offcol = Conf.cx - Conf.lmtcol+1; // move the offcol
	      Conf.cx = Conf.lmtcol - 1;
	    }
	  }
	}
      } else {
	Conf.cx -= 1;
      }
      break;
    case ARROW_RIGHT:
    case CTRL_L:
      if (row && filecol < row->size) {
	if (Conf.cx == Conf.lmtcol-1) { // cannot move
	  Conf.offcol++;
	} else {
	  Conf.cx += 1;
	}
      } else if (row && filecol == row->size) { // move to the next row
	Conf.cx = 0;
	Conf.offcol = 0;
	if (Conf.cy == Conf.lmtrow-1) {
	  Conf.offrow++;
	} else {
	  Conf.cy += 1;
	}
      }
      break;
    case ARROW_UP:
    case CTRL_K:
      if (Conf.cy == 0) {
	if (Conf.offrow) Conf.offrow--;
      } else {
	Conf.cy -= 1;
      }
      break;
    case ARROW_DOWN:
    case CTRL_J:
      if (filerow < Conf.numrows) {
	if (Conf.cy == Conf.lmtrow-1) {
	  Conf.offrow++;
	} else {
	  Conf.cy += 1;
	}
      }
      break;
  }

  filerow = Conf.offrow + Conf.cy;
  filecol = Conf.offcol + Conf.cx;

  /* Sometimes the cx will exceed the lmt chars of current line */
  row = (filerow >= Conf.numrows) ? NULL : &Conf.rows[filerow];
  rowlen = row ? row->size : 0;
  if (filecol > rowlen) {
    Conf.cx -= filecol-rowlen;
    if (Conf.cx < 0) {
      Conf.offcol += Conf.cx;
      Conf.cx = 0;
    }
  }
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

/* Append a char c in the command buffer (only one row) */
void Editor::InsertCommand(int c, editorConfig& Conf) {
  Conf.command += c;
};

/* Safe exit, with terminal clear and set the htty sane (raw mode) */
void Editor::Exit(int status) {
  std::system("clear");
  /* Why is this binary file transferred over "ssh -t" being changed? in unix */
  std::system("stty sane"); /* stty set the terminal driver */
  exit(status);
};

/* Make the command-line prompt by insert ":" into buffer */
void Editor::commandPrompt() {
  Conf.commandst = true;
  Conf.command = ":";
};

/* Transfer the current cmd into outside */
void Editor::transferCmd() {
  /* The c_str() method converts a string to an array of characters 
   * with a null character at the end. */
  this->command->msg = (char *) std::malloc(sizeof(Conf.command.c_str()));
  std::strcpy(this->command->msg, Conf.command.c_str()); // hard copy
  this->command->len = sizeof(Conf.command.c_str());
  this->command->valid = true;
  this->command->type = OUTCMD;
  Conf.command = ":";
  Conf.commandst = false; /* enter the editor input status */
};


/* Delete one char from the cmd buffer */
void Editor::delCharCmd() {
  if (!Conf.commandst || Conf.command.size() <= 1) { return; } // keep the ':'
  Conf.command.erase(Conf.command.size()-1);
}
