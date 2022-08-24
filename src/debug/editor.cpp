#include "debug.hpp"


void abAppend(struct abuf *ab, const char *s, int len) {
  char* heapn = (char*) realloc(ab->b, ab->len + len);
  if (heapn == NULL) return;
  memcpy(heapn+ab->len, s, len); // dst, src, len
  ab->b = heapn;
  ab->len += len;
};

void abFree(struct abuf *ab) {
  free(ab->b);
}

void Editor::RefreshScreen() {
  int y;
  editRow *r;
  char buf[32];
  struct abuf ab = ABUF_INIT;
  abAppend(&ab, "\x1b[?25l", 6);  // Hide cursor
  abAppend(&ab, "\x1b[H", 3);     // Go home

  for (y = 0; y < Conf.lmtrow; y++) { // 0
    int filerow = Conf.offrow + y;
    if (filerow >= Conf.numrows) { // init case (first time call)
      if (Conf.numrows == 0 && y == Conf.lmtrow/3) { // (print welcome msg in the 1/3 pos)
	char welcome[80];
	int welcomelen = snprintf(welcome, sizeof(welcome),
	    "Chainsaw debug editor -- version %s\x1b[0K\r\n]", CHAINSAW_VERSION);
	int padding = (Conf.lmtcol-welcomelen)/2;
	if (padding) {
	  abAppend(&ab, "~", 1);
	  padding--;
	}
	while (padding--) abAppend(&ab, " ", 1);
	abAppend(&ab, welcome, welcomelen);
      } else {
	  abAppend(&ab, "~\x1b[0K\r\n", 7); // ~ (empty)
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
	abAppend(&ab, c+j, 1);
      }
    }
    abAppend(&ab, "\x1b[39m", 5);
    abAppend(&ab, "\x1b[0K", 4);
    abAppend(&ab, "\r\n", 2);
  }

  /* Create a two row status. The first row */
  abAppend(&ab, "\x1b[0K", 4);
  abAppend(&ab, "\x1b[7m", 4);

  char status[80], rstatus[80];

  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
      Conf.filename, Conf.numrows, Conf.dirty ? "(modified)" : "");

  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
      Conf.offrow + Conf.cy + 1, Conf.numrows);

  if (len > Conf.lmtcol) len = Conf.lmtcol;
  abAppend(&ab, status, len);

  while (len < Conf.lmtcol) {
    if (Conf.lmtcol - len == rlen) {
      abAppend(&ab, rstatus, rlen);
    } else {
      abAppend(&ab, " ", 1);
      len++;
    }
  }

  abAppend(&ab, "\x1b[0m\r\n", 6);

  /* Second row depends on Conf.statusmsg and the status message update time */
  abAppend(&ab, "\x1b[0K", 4);
  int msglen = strlen(Conf.statusmsg);
  if (msglen && time(NULL) - Conf.statusmsg_time < 5)
    abAppend(&ab, Conf.statusmsg, msglen <= Conf.lmtcol ? msglen : Conf.lmtcol);


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
  abAppend(&ab, buf, strlen(buf));
  abAppend(&ab, "\x1b[?25h", 6); // show the cursor
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}
