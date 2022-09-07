#include "debug.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: debug <filename>\n";
    exit(1);
  }

  std::string execName = argv[1];

  execName += ".cpp";
  Inter* inter = new Inter(execName);
  Editor* editor = new Editor();

  inter->init();
  editor->init();

  // editor->Open(argv[1]);
  editor->EnableRawMode(STDIN_FILENO);
  // editor->SetStatusMessage( "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

  while (1) {
    editor->SetStatusMessage(inter->outbuffer->c_str());
    editor->RefreshScreen();
    editor->ProcessKeypress(STDIN_FILENO);
    if (editor->command->size() > 1) { /* consume */
      inter->command = editor->command;
      inter->Exec(*inter->command);
      editor->SetStatusMessage(inter->command->c_str());
      editor->command = new std::string("");
    }
  }
}
