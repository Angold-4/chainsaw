#include "debug.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: debug <filename>\n";
    exit(1);
  }

  std::string execName = argv[1];

  /* Shared buffer */
  buffer Command = BUFFINIT;
  buffer Outbuffer = BUFFINIT;

  execName += ".cpp";
  Inter* inter = new Inter(execName, &Outbuffer, &Command);
  Editor* editor = new Editor(&Outbuffer, &Command);

  inter->init();
  editor->init();

  // editor->Open(argv[1]);
  editor->EnableRawMode(STDIN_FILENO);
  // editor->SetStatusMessage( "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

  while (1) {
    if (Outbuffer.valid == true) { /* consume */
      editor->SetStatusMessage(Outbuffer.msg);
      Outbuffer = {NULL, 0, false};
    }

    editor->RefreshScreen();
    editor->ProcessKeypress(STDIN_FILENO);


    if (Command.valid == true) { /* consume */
      std::cout << Command.msg << '\n';
      inter->Exec(Command.msg);
      editor->SetStatusMessage(Command.msg);
      Command = {NULL, 0, false};
    }
    std::cout << "Hi there\n";
  }
}
