#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

const std::string WHITESPACE " \t\n\r\f\v";

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

void _freeArgs(char** args, int size) {
  for (int i = 0; i < size; i++) {
    free(args[i]);
  }
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

//Command methods
Command::Command(const char* cmd_line) : cmd_line(cmd_line) {}

const char* Command::getCmdLine() const {
  return cmd_line;
}

//BuiltInCommand methods
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}

//ChpromptCommand methods
ChpromptCommand::ChpromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute() {
  char* args[COMMAND_MAX_ARGS];
  int num_of_args = _parseCommandLine(cmd_line, args);
  SmallShell& smash = SmallShell::getInstance();
  if (num_of_args == 0) {
    smash.setPromptName(WHITESPACE);
  }
  else {
    smash.setPromptName(args[0]);
  }
  _freeArgs(args, num_of_args);
}

// ShowPidCommand methods
ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line)
{}

void ShowPidCommand::execute() {
  std::cout << "smash pid is " << getpid() << std::endl; //TODO: is endl or \n?
}

// GetCurrDirCommand methods
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line)
{}

void GetCurrDirCommand::execute() {
  char arr[COMMAND_MAX_PATH_LENGHT];
  if(getcwd(arr, COMMAND_MAX_PATH_LENGHT) != NULL) {
    std::cout << arr << std::endl;
  }
  else {
    perror("smash error: getcwd failed");
  }
}

// ChangeDirCommand methods
ChangeDirCommand::ChangeDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line)
{}

void ChangeDirCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();
  char* args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line, args);
  char* curr_dir = getcwd(args, size_args);
  if(!curr_dir) {
    perror("smash error: getcwd failed");
  }
  if (size_args > 2) {
    std::cerr << "smash error: cd: too many arguments" << std::endl;
  }
  else if(size_args == 1) { 
    // TODO: what if we only get cd? - piazza
  }
  else { // if size_args == 2
    if (args[1] == "-") {
      char* last_dir = smash.getLastDir();
        if (chdir(last_dir) == -1) {
          perror("smash error: chdir failed");
        }
        else {
          smash.setLastDir(curr_dir);
        }
      }
    else { // not "cd -"
      if(chdir(args[1]) == -1) {
        perror("smash error: chdir failed");
      }
      else {
          smash.setLastDir(curr_dir);
      }
    }
  } 
  _freeArgs(args, size_args); 
}

// JobsCommand methods
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line)
{}

void JobsCommand::execute() {
  SmallShell &smash = SmallShell::getInstance();
  smash->getJobsList()->printJobsList();
  getJobsList()->printJobsList();
}

// ForegroundCommand


// JobEntry methods
JobsList::JobEntry::JobEntry(int job_id, Command* cmd, pid_t job_pid) : job_id(jod_id), cmd(cmd), job_pid(job_pid) {}

pid_t JobsList::JobEntry::getJobPid() {
  return jobPid;
}

void JobsList::JobEntry::setJobId(int new_job_id) {
  jobId = new_job_id;
}

int JobsList::JobEntry::getJobId() {
  return jobId;
}

Command* JobsList::JobEntry::getCommand() {
  return cmd;
}

// JobsList methods
JobsList::JobsList() : jobs_list(new std::vector<JobEntry*>), max_job_id(0) {}

JobsList::~JobsList() {
  for (auto job_entry : jobs_list) {
    delete job_entry;
  }
  delete jobs_list;
}

void JobsList::addJob(Command* cmd) {
  removeFinishedJobs();
  max_job_id++;
  JobEntry* new_job = new JobEntry(max_job_id, cmd, getpid());
  jobs_list.insert(new_job);
  //TODO: Does the max_job_id change if the last job finished?
}

void JobsList::printJobsList() {
  removeFinishedJobs();
  for(auto job_entry : jobs_list) {
    cout << "[" << job_entry->getJobId() << "] " << job_entry->getCommand()->getCmdLine() << endl; 
  }
}

void JobsList::removeFinishedJobs() {
  for (JobEntry it = jobs_list.begin(); it != jobs_list.end(); it++) {
    JobEntry* tmp = *it;
    pid_t pid_status = waitpid(tmp->jobPid, nullptr, WNOHANG);
    if (pid_status == tmp->getJobPid() || pid_status == -1) {
      delete tmp;
      it = jobs_list.erase(it);
    }
    else { // pid_status == 0
      it++;
    }
  }
  updateMaxJobId();
}

void jobsList::updateMaxJobId() {
  if(jobs_list.empty()) {
    jobs_list.setJobId(0);
  }
  else {
    jobs_list.setJobId(jobs_list.back()->getJobId());
  }
}

std::vector<JobEntry*>* getJobsList() const {
  return *jobs_list;
}

//SmallShell methods
SmallShell::SmallShell() { // implement as singleton
  // TODO: add your implementation
}

std::string SmallShell::getPromptName() const {
    return prompt_name;
}

void SmallShell::setPromptName(const string& name) {
  if (name == WHITESPACE) {
    prompt_name = "smash> ";
  }
  prompt_name = _ltrim(name);
}

pid_t SmallShell::getShellPid() const {
  return this->shellPid;
}

void SmallShell::setShellPid(int pid) {
  this->shellPid = getpid();
}

void SmallShell::setLastDir(const char* new_dir) {
  last_dir = new_dir;
}

char* SmallShell::getLastDir() const {
  return last_dir;
}

JobsList* SmallShell::getJobsList() const {
  return jobs;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  string cmd_s = _trim(string(cmd_line)); // get rid of useless spaces
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  // TODO: add Special and External Commands
 // TODO: add isBuiltInCommand
  if(_isBackgroundComamnd(cmd_line)) {
    _removeBackgroundSign(const_cast<char*>(cmd_line));
  }
  if (firstWord.compare("chprompt") == 0) {
    return new ChpromptCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line);
  }
  else if (firstWord.compare("jobs") == 0) {
    return new JobsCommand(cmd_line);
  }
  else if (firstWord.compare("fg") == 0) {
    return new ForegroundCommand(cmd_line);
  }
  else if (firstWord.compare("quit") == 0) {
    return new QuitCommand(cmd_line);
  }
  else if (firstWord.compare("kill") == 0) {
    return new KillCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}
