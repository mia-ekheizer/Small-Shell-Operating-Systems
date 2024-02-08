#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include "sys/stat.h"

using namespace std;

const std::string WHITESPACE " \t\n\r\f\v";

#if 0
#define FUNC_ENTRY() \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args)
{
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for (std::string s; iss >> s;)
  {
    args[i] = (char *)malloc(s.length() + 1);
    memset(args[i], 0, s.length() + 1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

void _freeArgs(char **args, int size)
{
  for (int i = 0; i < size; i++)
  {
    free(args[i]);
  }
}

bool _isBackgroundComamnd(const char *cmd_line)
{
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line)
{
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos)
  {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&')
  {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

bool _isComplexCommand(const char *cmd_line)
{
  string as_string = string(cmd_line);
  return (as_string.find_first_of('*') != string::npos ||
          as_string.find_first_of('?') != string::npos);
}

// Command methods
Command::Command(const char *cmd_line) : cmd_line(cmd_line) {}

const char *Command::getCmdLine() const
{
  return cmd_line;
}

// BuiltInCommand methods
BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

// ChpromptCommand methods
ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute()
{
  char *args[COMMAND_MAX_ARGS];
  int num_of_args = _parseCommandLine(cmd_line, args);
  SmallShell &smash = SmallShell::getInstance();
  if (num_of_args == 1)
  { 
    smash.setPromptName(WHITESPACE);
  }
  else
  {
    smash.setPromptName(args[1]);
  }
  _freeArgs(args, num_of_args);
}

// ShowPidCommand methods
ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  std::cout << "smash pid is " << smash.getpid() << std::endl;
}

// GetCurrDirCommand methods
GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute()
{
  char arr[COMMAND_MAX_PATH_LENGHT];
  if (getcwd(arr, COMMAND_MAX_PATH_LENGHT) != NULL)
  {
    std::cout << arr << std::endl;
  }
  else
  {
    perror("smash error: getcwd failed");
  }
}

// ChangeDirCommand methods
ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line)
{
}

void ChangeDirCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line, args);
  char *curr_dir = getcwd();
  if (!curr_dir)
  {
    perror("smash error: getcwd failed");
  }
  if (size_args > 2)
  {
    std::cerr << "smash error: cd: too many arguments" << std::endl;
  }
  else
  { // if size_args == 2
    if (args[1] == "-")
    {
      char *last_dir = smash.getLastDir();
      if (chdir(last_dir) == -1)
      {
        perror("smash error: chdir failed");
      }
      else
      {
        smash.setLastDir(curr_dir);
      }
    }
    else
    { // not "cd -"
      if (chdir(args[1]) == -1)
      {
        perror("smash error: chdir failed");
      }
      else
      {
        smash.setLastDir(curr_dir);
      }
    }
  }
  _freeArgs(args, size_args);
}

// JobsCommand methods
JobsCommand::JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line)
{
}

void JobsCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  smash->getJobsList()->removeFinishedJobs();
  smash->getJobsList()->printJobsListWithId();
}

// ForegroundCommand
ForegroundCommand::ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ForegroundCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  JobsList *jobs = smash.getJobsList();
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line, args);
  if (size_args > 2 || typeof(args[1]) != typeof(int))
  {
    cerr << "smash error: fg: invalid arguments" << endl;
  }
  else if (size_args == 1)
  {
    if (jobs->isEmpty())
    {
      cerr << "smash error: fg: jobs list is empty" << endl;
    }
    else
    {
      JobsList::JobEntry *to_foreground = jobs->getLastJob();
      cout << to_foreground->getCommand()->getCmdLine() << " " << to_foreground->getJobPid() << endl;
      smash.setCurrFgProcess(to_foreground->getJobPid());
      jobs->removeJob(to_foreground);
    }
  }
  else
  { // size_args == 2
    JobsList::JobEntry *to_foreground = jobs->jobExistsInList(args[1]);
    if (!to_foreground)
    {
      cerr << "smash error: fg: job-id" << args[1] << "does not exist" << endl;
    }
    else
    {
      cout << to_foreground->getCommand()->getCmdLine() << " " << to_string(to_foreground->getJobPid()) << endl;
      smash.setCurrFgProcess(to_foreground->getJobPid());
      jobs->removeJob(to_foreground);
    }
  }
  _freeArgs(args, size_args);
}

// QuitCommand methods
QuitCommand::QuitCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

QuitCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  JobsList *jobs = smash.getJobsList();
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line, args);
  if (size_args > 1 && (strcmp(args[1], "kill") == 0))
  {
    std::cout << "smash: sending SIGKILL signal to " << jobs->getSize() << " jobs:" << std::endl;
    // killing all jobs:
    std::set<JobEntry *, CompareJobEntryUsingPid> jobsByPid; // new set, with a functor <
    for (JobEntry *job : jobs)
    { // fill the new set in an ordered way
      jobsByPid.insert(job);
    }

    for (JobEntry *job : jobsByPid)
    {
      if (kill(job->getJobPid(), SIGKILL) == -1)
      {
        perror("smash error: kill failed");
      }
      job->printJobPid();
    }
  }
  _freeArgs(args, size_args);
  delete this;
  exit(0);
}

// KillCommand methods
KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

KillCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line, args);
  if (size_args == 3) // if valid command size
  {
    if (*args[1] != '-') {
      std::cerr << "smash error: kill: invalid arguments" << std::endl;
      _freeArgs(args, size_args);
      return;
    }                                                 
    std::string sigNumStr = substr(str(args[1]), 1); // remove the "-" sign and take the rest of the string
    int jobId;
    int sigNum;
    try
    {
      jobId = std::stoi(str(args[2]));
      sigNum = std::stoi(sigNumStr);
    }
    catch (std::invalid_argument &e)
    {
      std::cerr << "smash error: kill: invalid arguments" << std::endl;
      _freeArgs(args, size_args);
      return;
    }
    // if command is valid:
    JobsList::JobEntry *job = smash.getJobsList()->getJobById(jobId);
    if (job == nullptr)
    {
      std::cerr << "smash error: kill: job-id " << jobId << " does not exist" << std::endl;
    }
    else
    { // TODO: finish this shit
      int status = kill(job->getJobPid(), sigNum);
      if (status == -1)
      {
        perror("smash error: kill failed");
      }
      else if (status == 0)
      {
        std::cout << "signal number " << sigNum << " was sent to pid " << job->getJobPid() << std::endl;
      }
    }
    else
    {
      std::cerr << "smash error: kill: invalid arguments" << endl;
    }
  }
  smash.updateMaxJobId();
  _freeArgs(args, size_args);
}

// ExternalCommand methods
ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute()
{
  bool is_background_command = false;
  if (_isBackgroundComamnd(cmd_line))
  {
    is_background_command = true;
    _removeBackgroundSign(cmd_line);
  }
  
  SmallShell &smash = SmallShell::getInstance();
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line, args);
  pid_t pid = fork();
  if (pid == -1)
  {
    perror("smash error: fork failed");
  }
  else if (pid == 0)
  { // child process executes the command.
    if (setpgrp() == -1)
    {
      perror("smash error: setpgrp failed");
    }
    else if (_isComplexCommand(cmd_line))
    {
      if (execlp("/bin/bash", "-c", cmd_line) == -1)
      {
        perror("smash error: execlp failed");
        exit(1);
      }
    }
    else // simple external command.
    {
      if (execvp(args[0], args) == -1)
      {
        perror("smash error: execvp failed");
        exit(1);
      }
    }
  }
  else
  { // parent process.
    if (is_background_command)
    { // adds the command to the jobs list if it is a background command, no waiting.
      smash.getJobsList()->addJob(this);
    }
    else
    { // if it is a foreground command, the parent waits for the child to finish.
      smash.setCurrFgPid(pid);
      if (waitpid(pid, nullptr) == -1)
      {
        perror("smash error: waitpid failed");
      }
      smash.setCurrFgPid(-1);
    }
  }
  _freeArgs();
}

// Special commands

// ChmodCommand methods
ChmodCommand::ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ChmodCommand::execute() {
  SmallShell &smash = SmallShell::getInstance();
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line, args);
  string err_msg = "smash error: chmod: invalid arguments";
  if (size_args > 3) {
    cerr << err_msg << endl;
  }
  else {
    mode_t new_mode;
    try {
      new_mode = stoi(args[1].c_str(), nullptr, 8); // convert the given mode to an octal base integer.
    } catch (invalid_argument &e) {
      cerr << err_msg << endl;
      _freeArgs();
      return;
    }
    if (chmod(args[2], new_mode) == -1) {
      perror("smash error: chmod failed");
    }
  }
  _freeArgs();
}

// JobEntry methods
JobsList::JobEntry::JobEntry(int job_id, Command *cmd, pid_t job_pid) : job_id(jod_id), cmd(cmd), job_pid(job_pid) {}

pid_t JobsList::JobEntry::getJobPid() const
{
  return jobPid;
}

void JobsList::JobEntry::setJobId(int new_job_id)
{
  jobId = new_job_id;
}

int JobsList::JobEntry::getJobId()
{
  return jobId;
}

void JobsList::JobEntry::printJobIdAndPid() const
{
  std::cout << "[" << this->getJobId() << "] " << this->getCommand()->getCmdLine() << " : " << this->getJobPid() << std::endl;
}

void JobsList::JobEntry::printJobPid() const
{
  std::cout << this->getJobPid() << ": " << this->getCommand()->getCmdLine() << std::endl;
}

Command *JobsList::JobEntry::getCommand()
{
  return cmd;
}

bool JobsList::CompareJobEntryUsingPid::operator()(const JobEntry *job1, const JobEntry *job2) const
{
  return job1->getJobPid() < job2->getJobPid();
}

// JobsList methods
JobsList::JobsList() : jobs_list(new std::vector<JobEntry *>), max_job_id(0) {}

JobsList::~JobsList()
{
  for (auto job_entry : jobs_list)
  {
    delete job_entry;
  }
  delete jobs_list;
}

void JobsList::addJob(Command *cmd)
{
  removeFinishedJobs();
  max_job_id++;
  JobEntry *new_job = new JobEntry(max_job_id, cmd, getpid());
  jobs_list.insert(new_job);
}

void JobsList::printJobsListWithId()
{ // for jobs command usage
  removeFinishedJobs();
  for (auto job_entry : jobs_list)
  {
    std::cout << "[" << job_entry->getJobId() << "] " << job_entry->getCommand()->getCmdLine() << std::endl;
  }
}

void JobsList::removeFinishedJobs()
{
  for (JobEntry it = jobs_list.begin(); it != jobs_list.end(); it++)
  {
    JobEntry *tmp = *it;
    if (kill(tmp->jobPid, 0) == -1) // job finished
    {
      delete tmp;
      it = jobs_list.erase(it);
    }
    else
    { // job not finished
      it++;
    }
  }
  updateMaxJobId();
}

JobsList::JobEntry *JobList::getJobById(int jobId)
{
  for (JobsList::JobEntry *it = jobs_list.begin(); it != jobs_list.end(); it++)
  {
    if (jobId == it->getJobId())
    {
      return it;
    }
  }
  return nullptr;
}

void JobsList::removeJobById(int jobId)
{
  for (JobsList::JobEntry *it = jobs_list.begin(); it != jobs_list.end(); it++)
  {
    if (jobId == it->getJobId())
    {
      delete it->getCommand();
      jobs_list.erase(it);
    }
  }
  updateMaxJobId();
}

void JobsList::updateMaxJobId()
{
  if (jobs_list.empty())
  {
    jobs_list.setJobId(0);
  }
  else
  {
    jobs_list.setJobId(jobs_list.back()->getJobId());
  }
}

int JobsList::getSize() const
{
  return jobs_list.size();
}

int JobsList::getMaxJobId() const
{
  return max_job_id;
}

void JobsList::setMaxJobId(int new_max_job_id)
{
  max_job_id = new_max_job_id;
}

std::vector<JobEntry *> *getJobsList() const
{
  return *jobs_list;
}

bool JobsList::isEmpty() const
{
  return JobsList.empty();
}

JobsList::JobEntry *JobsList::jobExistsInList(int job_id)
{
  for (JobsList::JobEntry *it = jobs_list.begin(); it != jobs_list.end(); it++)
  {
    if (job_id == it->jobId && waitpid(it->getJobPid(), nullptr, WNOHANG) != -1)
    {
      return it;
    }
  }
  return nullptr;
}

void JobsList::removeJob(JobsList::JobEntry *to_remove)
{
  for (JobsList::JobEntry it = jobs_list.begin(); it != jobs_list.end(); it++)
  {
    if (to_remove->getJobId() == it->getJobId())
    {
      delete to_remove->getCommand();
      jobs_list.erase(it);
    }
  }
}

JobsList::JobEntry *JobsList::getLastJob()
{
  return jobs_list.back();
}

// SmallShell methods
SmallShell::SmallShell() : shellPid(getpid()), last_dir(getcwd()), 
prompt_name("smash> "), jobs(new JobsList()), crr_fg_pid(-1)
{ // implement as singleton
}

/**
 * Creates and returns a pointer to Command class which matches the given command line (cmd_line)
 */
Command *SmallShell::CreateCommand(const char *cmd_line)
{                                        
  string cmd_s = _trim(string(cmd_line)); // get rid of useless spaces
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  /*
  if(isRedirectionCommand) ...
  else if (isPipeCommand)...
  else if (isTimeoutCommand) ... etc.
  */
  if (_isBuiltInCommand(firstWord))
  {
    _removeBackgroundSign(const_cast<char *>(cmd_line));
  }
  if (firstWord.compare("chprompt") == 0)
  {
    return new ChpromptCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0)
  {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0)
  {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0)
  {
    return new ChangeDirCommand(cmd_line);
  }
  else if (firstWord.compare("jobs") == 0)
  {
    return new JobsCommand(cmd_line);
  }
  else if (firstWord.compare("fg") == 0)
  {
    return new ForegroundCommand(cmd_line);
  }
  else if (firstWord.compare("quit") == 0)
  {
    return new QuitCommand(cmd_line);
  }
  else if (firstWord.compare("kill") == 0)
  {
    return new KillCommand(cmd_line);
  }
  else if (firstWord.compare("chmod") == 0)
  {
    return new ChmodCommand(cmd_line);
  }
  else
  {
    return new ExternalCommand(cmd_line);
  }
}

SmallShell::~SmallShell()
{
  delete jobs;
}

void SmallShell::executeCommand(const char *cmd_line)
{
  Command *cmd = CreateCommand(cmd_line);
  cmd->execute();
}

string SmallShell::getPromptName() const
{
  return prompt_name;
}

void SmallShell::setPromptName(const string &name)
{
  if (name == WHITESPACE)
  {
    prompt_name = "smash> ";
  }
  prompt_name = _ltrim(name);
}

pid_t SmallShell::getShellPid() const
{
  return this->shellPid;
}

void SmallShell::setShellPid(const pid_t shellPid)
{
  this->shellPid = getpid();
}

void SmallShell::setLastDir(const char *new_dir)
{
  last_dir = new_dir;
}

char *SmallShell::getLastDir() const
{
  return last_dir;
}

JobsList *SmallShell::getJobsList() const
{
  return jobs;
}

bool SmallShell::_isBuiltInCommand(string cmd_name)
{
  string copy_cmd = cmd_name;
  _removeBackgroundSign(copy_cmd);
  copy_cmd = _trim(copy_cmd);
  string firstWord = copy_cmd.substr(0, copy_cmd.find_first_of(" \n"));
  if (copy_cmd.compare("chprompt") == 0 || copy_cmd.compare("showpid") == 0 || copy_cmd.compare("pwd") == 0 ||
      copy_cmd.compare("cd") == 0 || copy_cmd.compare("jobs") == 0 || copy_cmd.compare("fg") == 0 ||
      copy_cmd.compare("quit") == 0 || copy_cmd.compare("kill") == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

pid_t SmallShell::getCurrFgPid() const {
  return curr_fg_pid;
}

void SmallShell::setCurrFgPid(const pid_t new_process_pid) {
  curr_fg_pid = new_process_pid;
}