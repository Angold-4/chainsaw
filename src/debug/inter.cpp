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
  int ret_code = 0;
  // std::string rsuffix = remove_suffix(filename);
  // cmd += filename + " -o " + rsuffix;
  cmd += this->execfile + " -o test";

  const char* debug_compile = cmd.c_str();

  ret_code = shellExec(debug_compile);

  if (ret_code) {
    std::cout << this->stderr_file_ << '\n';
    this->bufout->msg = (char*) std::malloc(sizeof(stderr_file_.c_str()));
    std::strcpy(this->bufout->msg, stderr_file_.c_str()); // hard copy
    bufout->len = stderr_file_.size();
    bufout->valid = true;
    bufout->type = BUFOUT;
    return false;
  }

  std::remove(this->stderr_file_.c_str());
  std::remove(this->stdout_file_.c_str());
  std::remove(this->tmp_dir_.c_str());

  this->valid_exec_ = false;

  /* Execute */
  /* TODO: time parser, get the load, execute time, mem usage */

  ret_code = shellExec("/usr/bin/time -l ./test");

  this->bufout->msg = (char*) std::malloc(sizeof(stderr_file_.c_str()));
  std::strcpy(this->bufout->msg, stderr_file_.c_str()); // hard copy
  bufout->len = stderr_file_.size();
  bufout->valid = true;
  bufout->type = BUFOUT;
  return false;

  this->bufout->msg = (char*) std::malloc(sizeof(stdout_file_.c_str()));
  std::strcpy(this->bufout->msg, stdout_file_.c_str()); // hard copy
  bufout->len = stdout_file_.size();
  bufout->valid = true;
  bufout->type = BUFOUT;
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

/* Execute the shell command, and get both its cerr and cout in tmp file */
int Inter::shellExec(const char* cmd) {
  std::string scmd(cmd);
  char tmp_dir[] = "/tmp/stdir.XXXXXX";
  mkdtemp(tmp_dir);
  mkdir(tmp_dir, 0777);
  this->stdout_file_ = std::string(tmp_dir) + "/stdout"; /* both filename */
  this->stderr_file_ = std::string(tmp_dir) + "/stderr";

  /* Execute the command: $cmd > stdout_file 2> stderr_file */

  std::string cli = scmd + " 2> " + stderr_file_ + " 1> " + stdout_file_;
  this->exit_code_ = std::system(cli.c_str());
  this->tmp_dir_ = std::string(tmp_dir);
  this->valid_exec_ = true;
  return exit_code_;
};

void Inter::RemoveTmp() {
  std::remove(this->stderr_file_.c_str());
  std::remove(this->stdout_file_.c_str());
  std::remove(this->tmp_dir_.c_str());
  this->valid_exec_ = false;
}
