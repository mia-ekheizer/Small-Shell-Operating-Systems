#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;
  SmallShell& smash = SmallShell::getInstance();
  pid_t curr_process_pid = smash.getCurrFgPid();
  if (curr_process_pid != -1) { // if there is no process running in the fg, then the smash ignores it.
    if (kill(curr_process_pid, SIGKILL) == -1) {
      perror("smash error: kill failed");
      return;
    }
    else { // killed fg process successfully
      cout << "smash: process " << curr_process_pid << " was killed" << endl;
    }
  }
}

void alarmHandler(int sig_num) {
  /* cout << "smash: got an alarm" << endl;
  // TODO: search for the command that caused the alarm, store it in alarm_cmd.
  if (kill(alarm_cmd.getPid(), SIGKILL) == -1) {
    perror("smash error: kill failed");
    exit(1);
  }
  else {
    cout << "smash: " << alarm_cmd.getCmdLine() << " timed out!" << endl;
  } */
}

