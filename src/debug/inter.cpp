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
    *this->errFree = COMPILE;
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


  /* Execute */
#ifdef MAC_OS
  ret_code = shellExec("/usr/bin/time -l ./test");
#endif

#ifdef LINUX
  ret_code = shellExec("/usr/bin/time -v ./test");
#endif

  if (ret_code) {
    *this->errFree = RUNTIME;
  } else {
    /* parse the output (time, max rss) */
#ifdef MAC_OS
    this->parseMac();
#endif

#ifdef LINUX
    this->parseLinux();
#endif
  }

  this->bufout->msg = (char*) std::malloc(sizeof(stderr_file_.c_str()));
  std::strcpy(this->bufout->msg, stderr_file_.c_str()); // hard copy
  bufout->len = stderr_file_.size();
  bufout->valid = true;
  bufout->type = BUFOUT;
  return true;

  /*
  this->bufout->msg = (char*) std::malloc(sizeof(stdout_file_.c_str()));
  std::strcpy(this->bufout->msg, stdout_file_.c_str()); // hard copy
  bufout->len = stdout_file_.size();
  bufout->valid = true;
  bufout->type = BUFOUT;
  return true;
  */
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
  return exit_code_;
};

void Inter::RemoveTmp() {
  std::remove(this->stderr_file_.c_str());
  std::remove(this->stdout_file_.c_str());
  std::remove(this->tmp_dir_.c_str());
};

void Inter::parseMac() {
  std::ifstream file(this->stderr_file_);
  std::stringstream buffer;
  buffer << file.rdbuf();

  std::string stdoe = buffer.str();
  std::string maxrss = "";
  std::string exetime = "";

  /* 1. get the max rss */
  int end = stdoe.find(MACKEY);
  if (end == std::string::npos ) {
    return;
  }

  for (int i = end-3; i > 0; i--) {
    char tmp = stdoe[i];
    if (tmp >= 48 && tmp <= 57 || tmp == '.') {
      /* is a digit */
      maxrss += tmp;
    } else {
      break;
    }
  }
  std::reverse(maxrss.begin(), maxrss.end());

  /* 2. get the exec time */
  int tend = stdoe.rfind(MACREAL, end);
  if (tend == std::string::npos) {
    return;
  }

  for (int i = tend-2; i > 0; i--) {
    char tmp = stdoe[i];
    if (tmp >= 48 && tmp <= 57 || tmp == '.') {
      /* is a digit */
      exetime += tmp;
    } else {
      break;
    }
  }
  std::reverse(maxrss.begin(), maxrss.end());


  /* 3. get out file */
  std::string out = stdoe.substr(0, tend-exetime.size()-1); /* the stdout */

  std::ofstream outfile;
  outfile.open(this->stderr_file_);
  outfile << out;
  outfile.close();

  out.clear();
  stdoe.clear();
  
  (*this->runtime)[0] = "MAC";
  (*this->runtime)[1] = maxrss.substr(0, maxrss.size()-3);
  (*this->runtime)[2] = exetime;
};

void Inter::parseLinux() {
<<<<<<< HEAD

=======
  std::ifstream file(this->stderr_file_);
  std::stringstream buffer;
  buffer << file.rdbuf();

  std::string stdoe = buffer.str();
  std::string maxrss = "";
  std::string exetime = "";

  /* 1. get the max rss */
  int end = stdoe.find(LINUXKEY);
  if (end == std::string::npos ) {
    return;
  }

  for (int i = end-3; i > 0; i--) {
    char tmp = stdoe[i];
    if (tmp >= 48 && tmp <= 57 || tmp == '.' || tmp == ':') {
      /* is a digit */
      maxrss += tmp;
    } else {
      break;
    }
  }

  std::reverse(maxrss.begin(), maxrss.end());

  /* 2. get the exec time */
  int tend = stdoe.rfind(LINUXSHARE, end);
  if (tend == std::string::npos) {
    return;
  }

  for (int i = tend-3; i > 0; i--) {
    char tmp = stdoe[i];
    if (tmp >= 48 && tmp <= 57 || tmp == '.' || tmp == ':') {
      /* is a digit */
      exetime += tmp;
    } else {
      break;
    }
  }

  std::reverse(exetime.begin(), exetime.end());

  int endi = stdoe.find(LINUXBEG);

  /* 3. get out file */
  std::string out = stdoe.substr(0, endi-1); /* the stdout */

  std::ofstream outfile;
  outfile.open(this->stderr_file_);
  outfile << out;
  outfile.close();

  out.clear();
  stdoe.clear();
  
  (*this->runtime)[0] = "LINUX";
  (*this->runtime)[1] = maxrss;
  (*this->runtime)[2] = exetime;
>>>>>>> e477cd274142a147aa0dadd6a22c80d8c78638da
};
