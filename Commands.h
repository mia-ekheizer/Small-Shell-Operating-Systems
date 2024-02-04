#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define COMMAND_MAX_PATH_LENGHT (80)

using namespace std;
class JobsList;

class Command {
 protected:
  const char* cmd_line;
// TODO: Add your data members
 public:
  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
  const char* getCmdLine() const;
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
  virtual void execute() = 0;
};

class ChpromptCommand : public BuiltInCommand {
  private:
  public:
  ChpromptCommand(const char* cmd_line);
  virtual ~ChpromptCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
  ChangeDirCommand(const char* cmd_line);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
private:
  JobsList* jobs;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
private:
  JobsList* jobs;

public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

//Special Commands

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
 public:
  ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class TimeoutCommand : public Command {
 public:
  TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class JobsList {  
 public:
  class JobEntry {
    private:
      int jobId;
      Command* cmd;
      pid_t jobPid;
    public:
   // TODO: Add your data members
    JobEntry(int job_id, Command* cmd, pid_t job_pid);
    ~JobEntry() {};
    pid_t getJobPid() const;
    void setJobId(int new_job_id);
    int getJobId() const;
    void printJobIdAndPid() const; //for jobsCommand::execute
    void printJobPid() const;  // for quitCommand::execute
    Command* getCommand() const;
  };
  class CompareJobEntryUsingPid { // needed for sorting jobs list by pid
    public:
      bool operator()(const JobEntry* job1, const JobEntry* job2) const;
  };
 private:
  // TODO: Add your data members
  std::vector<JobEntry*> jobs_list;
  int max_job_id;
 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd);
  void printJobsListWithIdandPid(); // for jobsCommand::execute
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
  void removeFinishedJobs();
  void updateMaxJobId();
  std::vector<JobEntry*>* getJobsList() const;
  bool isEmpty() const;
  JobEntry* jobExistsInList(int job_id);
  void removeJob(JobEntry* to_remove);
  JobEntry* getLastJob();
  int getSize() const;
  int getMaxJobId() const;
  void setMaxJobId(int new_max_job_id);
};

class SmallShell {
 private:
  // TODO: Add your data members
  pid_t shellPid;
  char* last_dir;
  std::string prompt_name = "smash> ";
  JobsList* jobs;
  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
  std::string getPromptName() const; // for chprompt usage
  
  void setPromptName(const std::string& name);  // for chprompt usage
  pid_t getShellPid() const; // for showpid usage

  void setLastDir(const char* new_dir); // for cd usage
  char* getLastDir() const; // for cd usage

  JobsList* getJobsList() const; // for jobs and fg usage

  bool _isBuiltInCommand(const string cmd_name);
};

#endif //SMASH_COMMAND_H_
