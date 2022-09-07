#include "debug.hpp"

/* Main entry function of Inter class */
bool Inter::Exec(char* cmd) {
  std::string ecmd(cmd);

  if (ecmd.length() <= 2) {
    set_buffer("Invalid Command");
    return false;
  }

  char cmdb = cmd[1];


  std::string scmd(cmd); // stack
  std::string filename = getFileName(scmd.substr(2));

  if (filename == " ") {
    set_buffer("Invalid Filename");
    return false;
  }

  switch (cmdb) {
    case '>':
      return infile(filename);
    case '<':
      return outexe(filename);
    default:
      set_buffer("Invalid Command");
      return false;
  }
};

/* Load the filename into the read buffer */
bool Inter::infile(std::string filename) {
  if (infiles.find(filename) == infiles.end()) {
    std::ofstream o("debug/" + filename);
    o << "dsd";
    o.close();
    set_buffer("Execute Successful!");
  }

  this->infilename = &filename;
  return true;
};

/* Execute the specific file */
bool Inter::outexe(std::string filename) {
  if (infiles.find(filename) == infiles.end()) {
    set_buffer("Invalid filename" + filename);
    return false;
  }

  /* Compile */
  std::string cmd = DEBUG_COMPILE; std::string res;
  std::string rsuffix = remove_suffix(filename);
  cmd += filename + " -o " + rsuffix;
  const char* debug_compile = cmd.c_str();

  res = shellExe(debug_compile);

  if (res != "")  {
    set_buffer("Compile Error:\n" + res);
    return false;
  }

  /* Execute */
  /* TODO: time parser, get the load, execute time, mem usage */
  return true;
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
