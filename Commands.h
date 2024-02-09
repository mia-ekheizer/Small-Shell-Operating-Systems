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
 public:
  Command(const char* cmd_line);
  virtual ~Command() {}
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
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
public:
  ChangeDirCommand(const char* cmd_line);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class JobsCommand : public BuiltInCommand {
 public:
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 public:
  ForegroundCommand(const char* cmd_line);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class QuitCommand : public BuiltInCommand {
public:
  QuitCommand(const char* cmd_line);
  virtual ~QuitCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
public:
  KillCommand(const char* cmd_line);
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
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override; //<-----TODO: WTF?(was commented out in the original code)
  //void cleanup() override;
};

class PipeCommand : public Command {
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
    JobEntry(int job_id, Command* cmd, pid_t job_pid);
    ~JobEntry() {};
    pid_t getJobPid() const;
    void setJobId(int new_job_id);
    int getJobId() const;
    void printJobIdAndPid() const;
    void printJobPid() const;  // for quitCommand
    Command* getCommand() const;
  };
  class CompareJobEntryUsingPid { // needed for sorting jobs list by pid
    public:
      bool operator()(const JobEntry* job1, const JobEntry* job2) const;
  };
 private:
  std::vector<JobEntry*> jobs_list;
  int max_job_id;
 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd);
  void printJobsListWithId(); // for jobsCommand
  void removeFinishedJobs();
  JobEntry* getJobById(int jobId);
  void removeJobById(int jobId);
  void updateMaxJobId();
  vector<JobEntry*>* getJobsList() const;
  bool isEmpty() const;
  JobEntry* jobExistsInList(int job_id);
  void removeJob(JobEntry* to_remove);
  JobEntry* getLastJob();
  int getSize() const;
  int getMaxJobId() const;
  void setMaxJobId(int new_max_job_id);
  void killAllJobsInList() const;
};

class SmallShell {
 private:
  pid_t shellPid;
  char* last_dir;
  string prompt_name = "smash";
  JobsList* jobs;
  pid_t curr_fg_pid = -1;
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
  string getPromptName() const; // for chprompt usage
  void setPromptName(const std::string& name);  // for chprompt usage
  pid_t getShellPid() const; // for showpid usage
  void setLastDir(char* new_dir); // for cd usage
  char* getLastDir() const;
  JobsList* getJobsList() const; // for jobs and fg usage
  pid_t getCurrFgPid() const;
  void setCurrFgPid(const pid_t new_process_pid);
};

#endif //SMASH_COMMAND_H_
