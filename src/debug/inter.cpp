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

  if (filename == " " || filename == "") {
    set_buffer("Invalid Command");
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
    set_buffer("Buffer Created: " + filename);
    infiles.insert(filename);
  } else {
    set_buffer("Load Buffer: " + filename);
  }

  filename = "debug/" + filename;

  std::ofstream o(filename, std::ios_base::app);
  this->outload->msg = (char*) std::malloc(sizeof(filename.c_str()));
  std::strcpy(this->outload->msg, filename.c_str()); // hard copy
  this->outload->len = sizeof(filename.c_str());
  this->outload->valid = true;
  this->outload->type = OUTLOAD;
  o.close();

  return true;
};

/* Execute the specific file */
bool Inter::outexe(std::string filename) {
  if (infiles.find(filename) == infiles.end()) {
    set_buffer("Invalid filename" + filename);
    return false;
  }

  /* Compile */

  std::string cmd = DEBUG_COMPILE; 
  std::string res;
  // std::string rsuffix = remove_suffix(filename);
  // cmd += filename + " -o " + rsuffix;
  cmd += this->execfile + " -o test";

  const char* debug_compile = cmd.c_str();

  res = shellExe(debug_compile);

  /* Execute */
  /* TODO: time parser, get the load, execute time, mem usage */

  res = shellExe("time ./test");

  if (res != "")  {
    bufout->msg = strdup(res.c_str());
    bufout->valid = true;
    bufout->len = res.size();
    bufout->type = BUFOUT;
  }

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
