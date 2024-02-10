#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include "sys/stat.h"
#include <set>
#include <sys/types.h>
#include <fcntl.h>

using namespace std;

const std::string WHITESPACE = " \t\n\r\f\v";

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

bool _isBuiltInCommand(const char* cmd_name)
{
  char* copy_cmd = (char*)malloc(string(cmd_name).length() + 1);
  copy_cmd = strcpy(copy_cmd, cmd_name);
  _removeBackgroundSign(copy_cmd);
  string cmd(copy_cmd);
  cmd = _trim(cmd);
  string firstWord = cmd.substr(0, cmd.find_first_of(" \n"));
  if (cmd.compare("chprompt") == 0 || cmd.compare("showpid") == 0 || cmd.compare("pwd") == 0 ||
      cmd.compare("cd") == 0 || cmd.compare("jobs") == 0 || cmd.compare("fg") == 0 ||
      cmd.compare("quit") == 0 || cmd.compare("kill") == 0)
  {
    free(copy_cmd);
    return true;
  }
  else
  {
    free(copy_cmd);
    return false;
  }
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
    smash.setPromptName("smash");
  }
  else
  {
    smash.setPromptName(args[0]);
  }
  _freeArgs(args, num_of_args);
}

// ShowPidCommand methods
ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  std::cout << "smash pid is " << smash.getShellPid() << std::endl;
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
  char path[COMMAND_MAX_PATH_LENGHT];
  char* curr_dir = getcwd(path, COMMAND_MAX_PATH_LENGHT);
  if (!curr_dir)
  {
    perror("smash error: getcwd failed");
  }
  if (size_args > 2)
  {
    std::cerr << "smash error: cd: too many arguments" << std::endl;
  }
  else if (size_args == 1)
  {
    // TODO: what if we only get cd? - piazza
  }
  else
  { // if size_args == 2
    if (!strcmp(args[1], "-"))
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
  smash.getJobsList()->removeFinishedJobs();
  smash.getJobsList()->printJobsListWithId();
}

// ForegroundCommand
ForegroundCommand::ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ForegroundCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  JobsList *jobs = smash.getJobsList();
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line, args);
  string invalid_args_msg = "smash error: fg: invalid arguments";
  int job_id;
  try {
    job_id = stoi(args[1]);
  } catch (invalid_argument &e) {
    cerr << invalid_args_msg << endl;
    return;
  }
  if (size_args > 2)
  {
    cerr << invalid_args_msg << endl;
    return;
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
      smash.setCurrFgPid(to_foreground->getJobPid());
      jobs->removeJob(to_foreground);
    }
  }
  else
  { // size_args == 2
    JobsList::JobEntry* to_foreground = jobs->jobExistsInList(job_id);
    if (!to_foreground)
    {
      cerr << "smash error: fg: job-id" << job_id << "does not exist" << endl;
    }
    else
    {
      cout << to_foreground->getCommand()->getCmdLine() << " " << to_string(to_foreground->getJobPid()) << endl;
      smash.setCurrFgPid(to_foreground->getJobPid());
      jobs->removeJob(to_foreground);
    }
  }
  _freeArgs(args, size_args);
}

// QuitCommand methods
QuitCommand::QuitCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void QuitCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  JobsList *jobs = smash.getJobsList();
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line, args);
  if (size_args > 1 && (strcmp(args[1], "kill") == 0))
  {
    std::cout << "smash: sending SIGKILL signal to " << jobs->getSize() << " jobs:" << std::endl;
    jobs->killAllJobsInList();
  }
  _freeArgs(args, size_args);
  delete this;
  exit(0);
}

// KillCommand methods
KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void KillCommand::execute()
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
    string sigNumStrFull(args[1]);                                            
    string sigNumStr = sigNumStrFull.substr(1); // remove the "-" sign and take the rest of the string
    int jobId;
    int sigNum;
    //TODO: what if we werent sent '-' before the integer 
    try
    {
      jobId = std::stoi(args[2]);
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
  }
  smash.getJobsList()->updateMaxJobId();
  _freeArgs(args, size_args);
}

// ExternalCommand methods
ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute()
{
  bool is_background_command = false;
  char* cmd_line_copy = (char*)malloc(string(cmd_line).length() + 1);
  cmd_line_copy = strcpy(cmd_line_copy, cmd_line);
  if (_isBackgroundComamnd(cmd_line))
  {
    is_background_command = true;
    _removeBackgroundSign(cmd_line_copy);
  }
  SmallShell& smash = SmallShell::getInstance();
  char *args[COMMAND_MAX_ARGS];
  const char* final_cmd = cmd_line_copy;
  int size_args = _parseCommandLine(cmd_line_copy, args);
  pid_t pid = fork();
  if (pid == -1)
  {
    free(cmd_line_copy);
    perror("smash error: fork failed");
    exit(1);
  }
  else if (pid == 0)
  { // child process executes the command.
    if (setpgrp() == -1)
    {
      free(cmd_line_copy);
      perror("smash error: setpgrp failed");
      exit(1);
    }
    else if (_isComplexCommand(final_cmd))
    {
      if (execlp("/bin/bash", "-c", final_cmd) == -1)
      {
        free(cmd_line_copy);
        perror("smash error: execlp failed");
        exit(1);
      }
    }
    else // simple external command.
    {
      if (execvp(args[0], args) == -1)
      {
        free(cmd_line_copy);
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
      if (waitpid(pid, nullptr, WUNTRACED) == -1)
      {
        free(cmd_line_copy);
        perror("smash error: waitpid failed");
        exit(1);
      }
      smash.setCurrFgPid(-1);
    }
  }
  free(cmd_line_copy);
  _freeArgs(args, size_args);
}

// Special commands
//IO RedirectionCommand methods
RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {}

void RedirectionCommand::execute() {
  SmallShell &smash = SmallShell::getInstance();
  int counter = 0;
  char* cmd_copy = strcpy(cmd_copy, cmd_line);
  while (*cmd_copy) {
    if (*cmd_copy == '>') {
      counter++;
    }
    cmd_copy++;
  }
  std::string command = cmd_line;
  std::string outfile;
  if(counter == 1) {
    command.substr(0, command.find(">"));
    outfile = command.substr(command.find(">") + 1, command.length());
  }
  else if(counter == 2) {
    command = command.substr(0, command.find(">>"));
    outfile = command.substr(command.find(">>") + 1, command.length());
  }
  else {
    std::cerr << "smash error: redirection: invalid arguments" << std::endl;
    return;
  }
  outfile = _trim(outfile);
  int fd = dup(1); // save the standard output
  if(fd == -1) {
    perror("smash error: dup failed");
    exit(1);
  }
  if(close(1) == -1) { //closing the standard output
    perror("smash error: close failed");
    exit(1);
  }

  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; //rw-r--r-- (644)
  if(counter == 1) { // > command
    if(open(outfile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, mode) == -1) {
      perror("smash error: open failed");
      //reopen standard output:
      if(dup2(fd, 1) == -1) {
        perror("smash error: dup2 failed");
      }
      else if(close(fd) == -1) { // dup2 succeeded, close the saved fd
        perror("smash error: close failed");
      }
      exit(1);
    }
  }
  else if(counter == 2) { // >> command
    if(open(outfile.c_str(), O_CREAT | O_WRONLY | O_APPEND, mode) == -1) {
      perror("smash error: open failed");
      //reopen standard output:
      if(dup2(fd, 1) == -1) {
        perror("smash error: dup2 failed");
      }
      else if(close(fd) == -1) { // dup2 succeeded, close the saved fd
        perror("smash error: close failed");
      }
      exit(1);
    }
  }
  //> or >> command succeeded, execute the command:
  smash.executeCommand(command.c_str());
  
  //close the file that we wrote into:
  if(close(1) == -1) {
    perror("smash error: close failed");
    exit(1);
  }
  //reopen standard output:
  if(dup2(fd, 1) == -1) {
    perror("smash error: dup2 failed");
    exit(1);
  }
  else if(close(fd) == -1) { // dup2 succeeded, close the saved fd
    perror("smash error: close failed");
    exit(1);
  }
}

bool isRedirectionCommand(const char *cmd_line) {
  std::string command = cmd_line;
  return (command.find(">") != std::string::npos || command.find(">>") != std::string::npos);
}

// ChmodCommand methods
ChmodCommand::ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ChmodCommand::execute() {
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line, args);
  string err_msg = "smash error: chmod: invalid arguments";
  if (size_args > 3) {
    cerr << err_msg << endl;
  }
  else {
    mode_t new_mode;
    try {
      new_mode = stoi(args[1], nullptr, 8); // convert the given mode to an octal base integer.
    } catch (invalid_argument &e) {
      cerr << err_msg << endl;
      _freeArgs(args, size_args);
      return;
    }
    if (chmod(args[2], new_mode) == -1) {
      perror("smash error: chmod failed");
      exit(1);
    }
  }
  _freeArgs(args, size_args);
}

// PipeCommand methods
PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {}

void PipeCommand::execute() {
  //TODO: implement
}

bool isPipeCommand(const char *cmd_line) {
  std::string command = cmd_line;
  return (command.find("|") != std::string::npos);
}

bool isChmodCommand(const char *cmd_line) {
  std::string command = cmd_line;
  return (command.find("chmod") != std::string::npos);
}

// JobEntry methods
JobsList::JobEntry::JobEntry(int job_id, Command *cmd, pid_t job_pid) : jobId(job_id), cmd(cmd), jobPid(job_pid) {}

pid_t JobsList::JobEntry::getJobPid() const
{
  return jobPid;
}

void JobsList::JobEntry::setJobId(int new_job_id)
{
  jobId = new_job_id;
}

int JobsList::JobEntry::getJobId() const
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

Command* JobsList::JobEntry::getCommand() const
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
  for (vector<JobsList::JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); it++)
  {
    delete *it;
  }
  delete jobs_list;
}

void JobsList::addJob(Command *cmd)
{
  removeFinishedJobs();
  max_job_id++;
  JobEntry* new_job = new JobEntry(max_job_id, cmd, getpid());
  jobs_list->push_back(new_job);
}

void JobsList::printJobsListWithId()
{ // for jobs command usage
  removeFinishedJobs();
  for (vector<JobsList::JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); it++)
  {
    JobsList::JobEntry* curr_job = *it;
    std::cout << "[" << curr_job->getJobId() << "] " << curr_job->getCommand()->getCmdLine() << std::endl;
  }
}

void JobsList::removeFinishedJobs()
{
  for (vector<JobsList::JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); it++)
  {
    JobEntry* tmp = *it;
    if (kill(tmp->getJobPid(), 0) == -1) // job finished
    {
      delete tmp;
      it = jobs_list->erase(it);
    }
    else
    { // job not finished
      it++;
    }
  }
  updateMaxJobId();
}

JobsList::JobEntry* JobsList::getJobById(int jobId)
{
  for (vector<JobsList::JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); it++)
  {
    JobsList::JobEntry* curr_job = *it;
    if (jobId == curr_job->getJobId())
    {
      return curr_job;
    }
  }
  return nullptr;
}

void JobsList::removeJobById(int jobId)
{
  for (vector<JobsList::JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); it++)
  {
    JobsList::JobEntry* curr_job = *it;
    if (jobId == curr_job->getJobId())
    {
      delete curr_job->getCommand();
      jobs_list->erase(it);
    }
  }
  updateMaxJobId();
}

void JobsList::updateMaxJobId()
{
  if (isEmpty())
  {
    setMaxJobId(0);
  }
  else
  {
    setMaxJobId(jobs_list->back()->getJobId());
  }
}

int JobsList::getSize() const
{
  return jobs_list->size();
}

int JobsList::getMaxJobId() const
{
  return max_job_id;
}

void JobsList::setMaxJobId(int new_max_job_id)
{
  max_job_id = new_max_job_id;
}

JobsList* SmallShell::getJobsList() const
{
  return jobs;
}

bool JobsList::isEmpty() const
{
  return jobs_list->empty();
}

JobsList::JobEntry *JobsList::jobExistsInList(int job_id)
{
  for (vector<JobsList::JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); it++)
  {
    JobsList::JobEntry* curr_job = *it;
    if (job_id == curr_job->getJobId() && waitpid(curr_job->getJobPid(), nullptr, WNOHANG) != -1)
    {
      return curr_job;
    }
  }
  return nullptr;
}

void JobsList::removeJob(JobsList::JobEntry *to_remove)
{
  for (vector<JobsList::JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); it++)
  {
    JobsList::JobEntry* curr_job = *it;
    if (to_remove->getJobId() == curr_job->getJobId())
    {
      delete to_remove->getCommand();
      jobs_list->erase(it);
    }
  }
}

JobsList::JobEntry *JobsList::getLastJob()
{
  return jobs_list->back();
}

void JobsList::killAllJobsInList() const {
  set<JobsList::JobEntry*, JobsList::CompareJobEntryUsingPid> jobsByPid; // new set, with a functor <
  for (vector<JobsList::JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); it++)
  { // fill the new set in an ordered way
    JobsList::JobEntry* curr_job = *it;
    jobsByPid.insert(curr_job);
  }
  for (JobsList::JobEntry* job : jobsByPid)
    {
      if (kill(job->getJobPid(), SIGKILL) == -1)
      {
        perror("smash error: kill failed");
      }
      job->printJobPid();
    }
}

// SmallShell methods
SmallShell::SmallShell() : shellPid(getpid()), last_dir(nullptr), 
prompt_name("smash"), jobs(new JobsList()), curr_fg_pid(-1) {}

SmallShell::~SmallShell() {
  delete jobs;
}

string SmallShell::getPromptName() const
{
  return prompt_name;
}

void SmallShell::setPromptName(const string &name)
{
  prompt_name = name;
}

pid_t SmallShell::getShellPid() const
{
  return this->shellPid;
}

void SmallShell::setLastDir(char *new_dir)
{
  last_dir = new_dir;
}

char *SmallShell::getLastDir() const
{
  return last_dir;
}

bool _isBuiltInCommand(string cmd_name)
{
  string copy_cmd = cmd_name;
  _removeBackgroundSign(copy_cmd);
  copy_cmd = _trim(copy_cmd);
  string firstWord = copy_cmd.substr(0, copy_cmd.find_first_of(" \n"));
  if (copy_cmd.compare("chprompt") == 0 || copy_cmd.compare("showpid") == 0 || copy_cmd.compare("pwd") == 0 ||
      copy_cmd.compare("cd") == 0 || copy_cmd.compare("jobs") == 0 || copy_cmd.compare("fg") == 0 ||
      copy_cmd.compare("quit") == 0 || copy_cmd.compare("kill") == 0){ 
        // TODO: add more built in commands
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * Creates and returns a pointer to Command class which matches the given command line (cmd_line)
 */
Command *SmallShell::CreateCommand(const char *cmd_line)
{                                         
  string cmd_s = _trim(string(cmd_line)); // get rid of useless spaces
  char* noBackgroundSignCommand = strcpy(noBackgroundSignCommand, cmd_s.c_str()); // prepare the command for the built in commands
  _removeBackgroundSign(noBackgroundSignCommand);
  string firstWord = noBackgroundSignCommand.substr(0, noBackgroundSignCommand.find_first_of(" \n"));

  if(isRedirectionCommand(cmd_line)) {
    return new RedirectionCommand(cmd_line);
  }
  else if(isPipeCommand(cmd_line)) { //TODO: implement isPipeCommand
    return new PipeCommand(cmd_line);
  }
  else if(isChmodCommand(cmd_line)) {
    return new ChmodCommand(cmd_line);
  }
  //if Built-in Commands
  else if (firstWord.compare("chprompt") == 0)
  { // need to accept spaces as the new prompt.
    return new ChpromptCommand(noBackgroundSignCommand);
  }
  else if (firstWord.compare("showpid") == 0)
  {
    return new ShowPidCommand(noBackgroundSignCommand);
  }
  else if (firstWord.compare("pwd") == 0)
  {
    return new GetCurrDirCommand(noBackgroundSignCommand);
  }
  else if (firstWord.compare("cd") == 0)
  {
    return new ChangeDirCommand(noBackgroundSignCommand);
  }
  else if (firstWord.compare("jobs") == 0)
  {
    return new JobsCommand(noBackgroundSignCommand);
  }
  else if (firstWord.compare("fg") == 0)
  {
    return new ForegroundCommand(noBackgroundSignCommand);
  }
  else if (firstWord.compare("quit") == 0)
  {
    return new QuitCommand(noBackgroundSignCommand);
  }
  else if (firstWord.compare("kill") == 0)
  {
    return new KillCommand(noBackgroundSignCommand);
  }
  else // external command - recieves the command line as is.
  {
    return new ExternalCommand(cmd_line);
  }
}


void SmallShell::executeCommand(const char *cmd_line)
{
  //check if there is a command to execute:
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line, args);
  if(size_args == 0) {
    return;
  }
  this->getJobsList()->removeFinishedJobs();

  Command *cmd;
  try {
    cmd = CreateCommand(cmd_line);
  } catch (std::bad_alloc &e) {
    perror("smash error: bad_alloc");
    _freeArgs(args, size_args);
    exit(1);
  }
  cmd->execute();
  delete cmd;
}

pid_t SmallShell::getCurrFgPid() const {
  return curr_fg_pid;
}

void setCurrFgPid(const pid_t new_process_pid) {
  curr_fg_pid = new_process;
}