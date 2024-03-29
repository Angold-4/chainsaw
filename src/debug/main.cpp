#include "debug.hpp"

void consume(buffer* buf, Inter* inter, Editor* editor) {
  if (!buf->valid) { return; }
  switch(buf->type) {
  case BUFNULL:
    break;
  case OUTBUF:
    /* Inter send status msg to user (Editor) */
    editor->SetStatusMessage(buf->msg);
    break;
  case OUTCMD:
    /* User (Editor) send command to Inter to execute */
    inter->Exec(buf->msg);
    break;
  case OUTLOAD:
    /* Inter indicate the load buffer (>) to user (Editor) */
    editor->Open(buf->msg);
    break;
  case BUFOUT:
    /* Inter indicate the execution output */
    editor->LoadOut(buf->msg);
    inter->RemoveTmp();
    break;
  default:
    break;
  }
  *buf = BUFFINIT;
};

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: chainsaw debug <question name>\n";
    exit(1);
  }

  std::string execName = argv[1];

  /* Shared buffer */
  buffer Outbuffer = BUFFINIT;/* Inter -> Editor */
  buffer Command = BUFFINIT;  /* Editor -> Inter */
  buffer OutLoad = BUFFINIT;  /* Inter -> Editor */
  buffer BufOut = BUFFINIT;   /* Inter -> Editor */

  int errFree = ERRFREE;      /* Inter -> Editor */

  VS runtime(3, "");         /* Inter -> Editor */

  execName += ".cpp";
  Inter* inter = new Inter(execName, &Outbuffer, &Command, &OutLoad, &BufOut, &errFree, &runtime);
  Editor* editor = new Editor(&Outbuffer, &Command, &errFree, &runtime);

  inter->init();
  editor->init();

  // editor->Open(argv[1]);
  editor->EnableRawMode(STDIN_FILENO);
  editor->SetStatusMessage( "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-C = command");

  while (1) {
    consume(&Outbuffer, inter, editor);
    consume(&BufOut, inter, editor);
    consume(&OutLoad, inter, editor);
    editor->RefreshScreen();
    editor->ProcessKeypress(STDIN_FILENO);
    consume(&Command, inter, editor);
  }
}
