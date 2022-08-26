#include "debug.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: debug <filename>\n";
    exit(1);
  }

  Editor *editor = new Editor();
  editor->init();
  editor->Open(argv[1]);
  editor->EnableRawMode(STDIN_FILENO);
  editor->RefreshScreen();
}
