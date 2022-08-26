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
	// abAppend(&ab, welcome, welcomelen);
	buffer += *welcome;
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
  buffer.push_back(*status);

  while (len < Conf.lmtcol) {
    if (Conf.lmtcol - len == rlen) {
      buffer.push_back(*rstatus);
    } else {
      buffer += " ";
      len++;
    }
  }

  buffer += "\x1b[0m\r\n";

  /* Second row depends on Conf.statusmsg and the status message update time */
  buffer += "\x1b[0K";
  int msglen = strlen(Conf.statusmsg);
  if (msglen && time(NULL) - Conf.statusmsg_time < 5)
    // abAppend(&ab, Conf.statusmsg, msglen <= Conf.lmtcol ? msglen : Conf.lmtcol);
    buffer.push_back(*Conf.statusmsg);


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

  snprintf(buf, sizeof(buf), "\x1b[%d; %dH", Conf.cy+1, cx);
  buffer.push_back(*buf);
  buffer += "\x1b[?25h"; // show the cursor
  write(STDOUT_FILENO, &buffer, buffer.size());
}

/* Load the specified program in the editor memory and returns 0 on success */
int Editor::Open(char* filename) {
  FILE* fp;
  Conf.dirty = 0;
  size_t fnlen = strlen(filename) + 1;
  Conf.filename = new char[fnlen];
  delete[] Conf.filename;
  memcpy(Conf.filename, filename, fnlen);

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
    /* Insert the row into middle, shifting other rows on the botoon */
    memmove(Conf.rows+pos+1, Conf.rows+pos, sizeof(Conf.rows[0]) * (Conf.numrows-pos));
    for (int j = pos+1; j <= Conf.numrows; j++) Conf.rows[j].idx++; // update the index
  }

  Conf.rows[pos].size = len;
  Conf.rows[pos].chars = new char[len];
  memcpy(Conf.rows[pos].chars, line, len+1);
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
  std::cout << erow->render << std::endl;
};
