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

const std::string WHITESPACE = " \n\r\t\f\v";

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
  cmd_line[idx] = ' ';
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

bool _isComplexCommand(const string &cmd_line)
{
  return cmd_line.find_first_of("*?") != std::string::npos;
}

bool _isBuiltInCommand(const char* cmd_line)
{
  char copy_cmd[COMMAND_ARGS_MAX_LENGTH];
  strcpy(copy_cmd, cmd_line);
  _removeBackgroundSign(copy_cmd);
  string cmd = string(copy_cmd);
  cmd = _trim(cmd);
  string firstWord = cmd.substr(0, cmd.find_first_of(" \n"));
  if (firstWord.compare("chprompt") == 0 || firstWord.compare("showpid") == 0 || firstWord.compare("pwd") == 0 ||
      firstWord.compare("cd") == 0 || firstWord.compare("jobs") == 0 || firstWord.compare("fg") == 0 ||
      firstWord.compare("quit") == 0 || firstWord.compare("kill") == 0)
    return true;
  else
    return false;
  
}

// Command methods
Command::Command(const char *cmd_line) : cmd_line(cmd_line) {}

// BuiltInCommand methods
BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

// ChpromptCommand methods
ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute()
{
  char *args[COMMAND_MAX_ARGS];
  int num_of_args = _parseCommandLine(cmd_line.c_str(), args);
  SmallShell &smash = SmallShell::getInstance();
  if (num_of_args == 1)
  { 
    smash.setPromptName("smash");
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
ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line.c_str(), args);
  char path[COMMAND_MAX_PATH_LENGHT];
  if (size_args > 2)
  {
    std::cerr << "smash error: cd: too many arguments" << std::endl;
  }
  else if (size_args == 1)
  {
    _freeArgs(args,size_args);
    return;
  }
  else if (size_args == 2)
  {
    if (!strcmp(args[1], "-")){ // if cd -
      if(smash.getLastDir() == "") { // if last directory not set
        std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
      }
      else { //if there is a last directory
        if (!getcwd(path, COMMAND_MAX_PATH_LENGHT)){
          perror("smash error: getcwd failed");
        }
        string curr_dir = path;
        if (chdir(smash.getLastDir().c_str()) == -1){
          perror("smash error: chdir failed");
        }
        else{
          smash.setLastDir(curr_dir);
        }
      }
    }
    else
    { // not "cd -"

      if (!getcwd(path, COMMAND_MAX_PATH_LENGHT)){
        perror("smash error: getcwd failed");
      }
      string curr_dir = path;
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
  int size_args = _parseCommandLine(cmd_line.c_str(), args);
  int job_id;
  if (size_args >= 2) {
    try {
      job_id = stoi(string(args[1]));
    } catch (const invalid_argument &e) {
      std::cerr << "smash error: fg: invalid arguments" << std::endl;
      _freeArgs(args,size_args);
      return;
    }
    JobsList::JobEntry* to_foreground = jobs->getJobById(job_id);
    if (!to_foreground) { // if no job with that id
      std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << std::endl;
      _freeArgs(args,size_args);
      return;
    }
    else {
      if (size_args > 2) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        _freeArgs(args, size_args);
        return;
      }
      std::cout << to_foreground->getCommand() << " " << to_string(to_foreground->getJobPid()) << std::endl;
      smash.setCurrFgPid(to_foreground->getJobPid());
      int status;
      if(waitpid(to_foreground->getJobPid(),&status,WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        _freeArgs(args,size_args);
        return;
      }
      jobs->removeJob(to_foreground);
    }
  }
  else if (size_args == 1){
    if (jobs->isEmpty()){
      cerr << "smash error: fg: jobs list is empty" << endl;
      _freeArgs(args,size_args);
      return;
    }
    else{
      JobsList::JobEntry* to_foreground = jobs->getJobById(jobs->getMaxJobId());
      std::cout << to_foreground->getCommand() << " " << to_string(to_foreground->getJobPid()) << std::endl;
      smash.setCurrFgPid(to_foreground->getJobPid());
      int status;
      if(waitpid(to_foreground->getJobPid(),&status,WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        _freeArgs(args,size_args);
        return;
      }
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
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line.c_str(), args);
  if (size_args > 1 && (strcmp(args[1], "kill") == 0))
  {
    std::cout << "smash: sending SIGKILL signal to " << smash.getJobsList()->getSize() << " jobs:" << std::endl;
    smash.getJobsList()->killAllJobsInList();
  }
  _freeArgs(args, size_args);
  delete this;
  exit(0);
}

// KillCommand methods
KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

// if size >= 3 -> check if jobId is int, if yes-> check if job exists->continue; if not->invalid args
void KillCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line.c_str(), args);
  if(size_args <= 2) {
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    _freeArgs(args, size_args);
    return; 
  } 
  else // if size_args >= 3
  {
    int jobId;
    int sigNum;
    try //check if jobId is int
    {
      jobId = std::stoi(args[2]); 
    }
    catch (std::invalid_argument &e)
    {
      std::cerr << "smash error: kill: invalid arguments" << std::endl;
      _freeArgs(args, size_args);
      return;
    }

    // if JobId is int:
    JobsList::JobEntry *job = smash.getJobsList()->getJobById(jobId); // check if the job exists
    if (job == nullptr)
    {
      std::cerr << "smash error: kill: job-id " << jobId << " does not exist" << std::endl;
      _freeArgs(args, size_args);
      return;
    }

    std::string sigNumStrFull(args[1]);
    if(sigNumStrFull[0] != '-') {
      std::cerr << "smash error: kill: invalid arguments" << std::endl;
      _freeArgs(args, size_args);
      return;
    }
    std::string sigNumStr = sigNumStrFull.substr(1); // remove the "-" sign and take the rest of the string
    try {
      sigNum = std::stoi(sigNumStr); // check if the signal is an int
    }
    catch (std::invalid_argument &e) {
      std::cerr << "smash error: kill: invalid arguments" << std::endl;
      _freeArgs(args, size_args);
      return;
    }                                    
    if (size_args > 3) {
      std::cerr << "smash error: kill: invalid arguments" << std::endl;
      _freeArgs(args, size_args);
      return;
    }
    else
    { 
      int status = kill(job->getJobPid(), sigNum);
      if (status == -1){
        perror("smash error: kill failed");
        _freeArgs(args, size_args);
        return;
      }
      else if (status == 0){
        std::cout << "signal number " << sigNum << " was sent to pid " << job->getJobPid() << std::endl;
        smash.getJobsList()->removeFinishedJobs();
        _freeArgs(args, size_args);
        return;
      }
    }
  }
}

// ExternalCommand methods
ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute()
{
  char cmd_line_copy[COMMAND_ARGS_MAX_LENGTH];
  strcpy(cmd_line_copy, cmd_line.c_str());
  bool is_background_command = false;
  bool is_complex_command = _isComplexCommand(cmd_line.c_str());
  if (_isBackgroundComamnd(cmd_line.c_str()))
  {
    is_background_command = true;
    _removeBackgroundSign(cmd_line_copy);
  }
  SmallShell& smash = SmallShell::getInstance();
  char *args[COMMAND_MAX_ARGS];
  const char* final_cmd = cmd_line_copy;
  int size_args = _parseCommandLine(cmd_line_copy, args);

  //TODO: ifPipe?
  pid_t pid = fork();
  if (pid == -1)
  {
    perror("smash error: fork failed");
    //exit(1); //TODO: is needed?
  }
  else if (pid == 0)
  { // child process executes the command.
    if (setpgrp() == -1)
    {
      perror("smash error: setpgrp failed");
      exit(1);
    }
    else if (is_complex_command)
    {
      if (execlp("bash","bash", "-c", final_cmd,NULL) == -1)
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
      smash.getJobsList()->addJob(string(cmd_line), pid);
    }
    else
    { // if it is a foreground command, the parent waits for the child to finish.
      smash.setCurrFgPid(pid);
      if (waitpid(pid, nullptr, WUNTRACED) == -1)
      {
        perror("smash error: waitpid failed");
        //exit(1); //TODO: is needed?
      }
      smash.setCurrFgPid(-1);
    }
  }
  _freeArgs(args, size_args);
}

// Special commands
//IO RedirectionCommand methods
RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {}

void RedirectionCommand::execute() {
  SmallShell &smash = SmallShell::getInstance();
  int counter = 0;
  for(char c: cmd_line) {
    if (c == '>') {
      counter++;
    }
  }
  std::string command = cmd_line;
  std::string outfile;
  if(counter == 1) {
    command = cmd_line.substr(0, cmd_line.find(">"));
    outfile = cmd_line.substr(cmd_line.find(">") + 1, cmd_line.length());
  }
  else if(counter == 2) {
    command = cmd_line.substr(0, cmd_line.find(">>"));
    outfile = cmd_line.substr(cmd_line.find(">>") + 2, cmd_line.length());
  }
  else {
    std::cerr << "smash error: redirection: invalid arguments" << std::endl;
    return;
  }
  outfile = _trim(outfile);
  int fd = dup(1); // save the standard output
  if(fd == -1) {
    perror("smash error: dup failed");
    return;
  }
  if(close(1) == -1) { //closing the standard output
    perror("smash error: close failed");
    return;
  }

  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH; 
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
      return;
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
      return;
    }
  }
  //> or >> command succeeded, execute the command:
  smash.executeCommand(command.c_str());
  
  //close the file that we wrote into:
  if(close(1) == -1) {
    perror("smash error: close failed");
  }
  //reopen standard output:
  if(dup2(fd, 1) == -1) {
    perror("smash error: dup2 failed");
  }
  else if(close(fd) == -1) { // dup2 succeeded, close the saved fd
    perror("smash error: close failed");
  }
  return;
}

bool isRedirectionCommand(const char *cmd_line) {
  std::string command = cmd_line;
  return (command.find_first_of(">") != std::string::npos || command.find_first_of(">>") != std::string::npos);
}

// ChmodCommand methods
ChmodCommand::ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ChmodCommand::execute() {
  char *args[COMMAND_MAX_ARGS];
  int size_args = _parseCommandLine(cmd_line.c_str(), args);
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
  SmallShell &smash = SmallShell::getInstance();
  smash.setIsPipe(true);
  bool is_stderr = false;
  std::string cmd_line_str = cmd_line;
  std::string first_command;
  std::string second_command;
  if(cmd_line_str.find("|&") != std::string::npos) {
    first_command = cmd_line.substr(0, cmd_line.find("|&"));
    second_command = cmd_line.substr(cmd_line.find("|&") + 2, cmd_line.length())
    is_stderr = true;
  }
  else {
    first_command = cmd_line.substr(0, cmd_line.find("|"));
    second_command = cmd_line.substr(cmd_line.find("|") + 1, cmd_line.length())
  }
  int stdin_copy = dup(0);
  int stdout_copy = dup(1);
  int stderr_copy = dup(2);
  if(stdin_copy == -1 || stdout_copy == -1 || stderr_copy == -1) {
    perror("smash error: dup failed");
    return;
  }
  int pipefd[2];
  if(pipe(pipefd) == -1) {
    perror("smash error: pipe failed");
    return;
  }

  //decide if we pipe the stderr or the stdout:
  if(is_stderr) {
    if(dup2(pipefd[1], 2) == -1) {
      perror("smash error: dup2 failed");
      closePipe(pipefd);
      restoreDup(stdin_copy, stdout_copy, stderr_copy);
      return;
    }
  }
  else { //is_stdout
    if(dup2(pipefd[1], 1) == -1) {
      perror("smash error: dup2 failed");
      closePipe(pipefd);
      restoreDup(stdin_copy, stdout_copy, stderr_copy);
      }
      return;
    }
  

  pid_t pid = fork();
  if(pid == -1) { 
    perror("smash error: fork failed");
    //restore the standard input and output:
    restoreDup(stdin_copy, stdout_copy, stderr_copy);
    closePipe(pipefd);
  }
  else if(pid == 0) {
    if(setpgrp() == -1) {
      perror("smash error: setpgrp failed");
      exit(1);
    }
    closePipe(pipefd);
    smash.executeCommand(first_command.c_str());
    exit(0);
  }

  //parent process:  (executes the second command)

  //prepare to read from the pipe:
  if(dup2(pipefd[0], 0) == -1) {
    perror("smash error: dup2 failed");
    closePipe(pipefd);
    restoreDup(stdin_copy, stdout_copy, stderr_copy);
    return;
  }
  //restore stderr and stdout:
  if(is_stderr) {
    if(dup2(stderr_copy, 2) == -1) {
      perror("smash error: dup2 failed");
      closePipe(pipefd);
      return;
    }
  }
  else { //is_stdout
    if(dup2(stdout_copy, 1) == -1) {
      perror("smash error: dup2 failed");
      closePipe(pipefd);
      return;
    }
  }
  //execute the second command:
  pid_t pid2 = fork();
  if(pid2 == -1) {
    perror("smash error: fork failed");
    closePipe(pipefd);
    restoreDup(stdin_copy, stdout_copy, stderr_copy);
  }
  else if(pid2 == 0) {
    if(setpgrp() == -1) {
      perror("smash error: setpgrp failed");
      exit(1);
    }
    closePipe(pipefd);
    smash.executeCommand(second_command.c_str());
    exit(0);
  }
  //wait for the child processes to finish:
  //TODO: continue here
}

bool isPipeCommand(const string &cmd_line) {
  return (cmd_line.find_first_of("|") != std::string::npos);
}

// JobEntry methods
JobsList::JobEntry::JobEntry(int job_id, std::string cmd_line, pid_t job_pid) : jobId(job_id), cmd_line(cmd_line), jobPid(job_pid) {}

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

void JobsList::JobEntry::printJobPid() const
{
  std::cout << this->getJobPid() << ": " << this->getCommand() << std::endl;
}

string JobsList::JobEntry::getCommand() const
{
  return cmd_line;
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
    JobsList::JobEntry* to_delete = *it;
    delete to_delete;
  }
  delete jobs_list;
}

void JobsList::addJob(string cmd_line, pid_t pid)
{
  removeFinishedJobs();
  setMaxJobId(getMaxJobId()+1);
  JobsList::JobEntry* new_job = new JobsList::JobEntry(max_job_id, cmd_line, pid);
  jobs_list->push_back(new_job);
}

void JobsList::printJobsListWithId()
{ // for jobs command usage
  removeFinishedJobs();
  for (vector<JobsList::JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end(); it++)
  {
    JobsList::JobEntry* curr_job = *it;
    std::cout << "[" << curr_job->getJobId() << "] " << curr_job->getCommand() << std::endl;
  }
}

void JobsList::removeFinishedJobs()
{
  if (isEmpty()) {
    updateMaxJobId();
    return;
  }
  for (vector<JobsList::JobEntry*>::iterator it = jobs_list->begin(); it != jobs_list->end();)
  {
    JobsList::JobEntry* tmp = *it;
    if (waitpid(tmp->getJobPid(), nullptr, WNOHANG) == tmp->getJobPid()) // job finished
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
      delete curr_job;
      jobs_list->erase(it);
      break;
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
      delete curr_job;
      jobs_list->erase(it);
      return;
    }
  }
}

JobsList::JobEntry *JobsList::getLastJob()
{
  return jobs_list->back();
}

void JobsList::killAllJobsInList()  {
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
    for (JobsList::JobEntry* job : jobsByPid)
    {
      removeJob(job);
    }
    setMaxJobId(0);
}

// SmallShell methods
SmallShell::SmallShell() : shellPid(getpid()), last_dir(""), 
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

void SmallShell::setLastDir(const string &new_dir)
{
  last_dir = new_dir;
}

string SmallShell::getLastDir() const
{
  return last_dir;
}

/**
 * Creates and returns a pointer to Command class which matches the given command line (cmd_line)
 */
Command *SmallShell::CreateCommand(const char *cmd_line)
{
  char cmd_line_noBS[COMMAND_ARGS_MAX_LENGTH];
  strcpy(cmd_line_noBS, cmd_line); // prepare the command for the built in commands
  std::string trimmed_for_pipe = _trim(string(cmd_line_noBS));//(has backsign)
  _removeBackgroundSign(cmd_line_noBS);
  std::string trimmed_cmd = _trim(string(cmd_line_noBS)); 
  string firstWord = trimmed_cmd.substr(0, trimmed_cmd.find_first_of(" \n"));
  if(isRedirectionCommand(cmd_line_noBS)) {
    return new RedirectionCommand(cmd_line_noBS);
  }
  else if(isPipeCommand(cmd_line_noBS)) { 
    return new PipeCommand(trimmed_for_pipe.c_str());//TODO: check if right
  }
  else if(firstWord.compare("chmod") == 0) {
    return new ChmodCommand(cmd_line_noBS);
  }
  //if Built-in Commands
  else if (firstWord.compare("chprompt") == 0)
  {
    return new ChpromptCommand(cmd_line_noBS);
  }
  else if (firstWord.compare("showpid") == 0)
  {
    return new ShowPidCommand(cmd_line_noBS);
  }
  else if (firstWord.compare("pwd") == 0)
  {
    return new GetCurrDirCommand(cmd_line_noBS);
  }
  else if (firstWord.compare("cd") == 0)
  {
    return new ChangeDirCommand(cmd_line_noBS);
  }
  else if (firstWord.compare("jobs") == 0)
  {
    return new JobsCommand(cmd_line_noBS);
  }
  else if (firstWord.compare("fg") == 0)
  {
    return new ForegroundCommand(cmd_line_noBS);
  }
  else if (firstWord.compare("quit") == 0)
  {
    return new QuitCommand(cmd_line_noBS);
  }
  else if (firstWord.compare("kill") == 0)
  {
    return new KillCommand(cmd_line_noBS);
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
    _freeArgs(args,size_args);
    return;
  }
  this->getJobsList()->removeFinishedJobs();

  Command *cmd;
  try {
    cmd = CreateCommand(cmd_line);
  } catch (const std::bad_alloc &e) {
    perror("smash error: bad_alloc");
    _freeArgs(args, size_args);
    return;
  }
  if(cmd) {
    cmd->execute();
  }
  delete cmd;
}

pid_t SmallShell::getCurrFgPid() const {
  return curr_fg_pid;
}

void SmallShell::setCurrFgPid(const pid_t new_process_pid) {
  curr_fg_pid = new_process_pid;
}

void SmallShell::setCurrCommand(const std::string& command) {
  curr_command = command;
}

std::string SmallShell::getCurrCommand()const {
  return curr_command;
}

void SmallShell::setIsPipe(const bool isPipe) {
  this->isPipe = isPipe;
}

bool SmallShell::getIsPipe() const {
  return this->isPipe;
}
void closePipe(int *pipefd) {
  if(close(pipefd[0]) == -1 || close(pipefd[1]) == -1) {
    perror("smash error: close failed");
  }
}
void restoreDup(int stdin_copy, int stdout_copy, int stderr_copy) {
  if(dup2(stdin_copy, 0) == -1 || dup2(stdout_copy, 1) == -1 || dup2(stderr_copy, 2) == -1) {
    perror("smash error: dup2 failed");
  }
}