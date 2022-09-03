#include "debug.hpp"

/* Main entry function of Inter class */
void Inter::Exec(std::string cmd) {
  this->command = cmd;

  if (cmd.length() <= 2) {
    this->error = true;
    this->outbuffer = "Invalid command1";
    return;
  }

  char cmdb = cmd[0];

  std::string filename = getFileName(cmd.substr(1));

  if (filename == " ") {
    this->error = true;
    this->outbuffer = "Invalid command2";
    return;
  }

  switch (cmdb) {
    case '>':
      infile(filename);
      break;
    case '<':
      outexe(filename);
      break;
    default:
      this->error = true;
      this->outbuffer = cmd;
      return;
  }
};

/* Load the filename into read buffer */
void Inter::infile(std::string filename) {
  if (infiles.find(filename) == infiles.end()) {
    std::ofstream o("debug/" + filename);
    o << "dsd";
    o.close();
  }
  this->outbuffer = "Execute Successful";
  this->infilename = filename;
  this->error = false;
};

/* Execute the specific file */
void Inter::outexe(std::string filename) {
  if (infiles.find(filename) == infiles.end()) {
    this->error = true;
    this->outbuffer = 
      "Invalid filename: " + filename;
    return;
  }

  /* Compile */
  std::string cmd = DEBUG_COMPILE; std::string res;
  std::string rsuffix = remove_suffix(filename);
  cmd += filename + " -o " + rsuffix;
  const char* debug_compile = cmd.c_str();

  res = shellExe(debug_compile);

  if (res != "")  {
    this->error = true;
    this->outbuffer = "";
    this->outbuffer += "Compile Error:\n" + res;
    return;
  }

  /* Execute */
  /* TODO: time parser, get the load, execute time, mem usage */
};

/* Execute the shell command and get its return */
std::string Inter::shellExe(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
};
