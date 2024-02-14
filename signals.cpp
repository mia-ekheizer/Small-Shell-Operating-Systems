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
      smash.setCurrFgPid(-1);
    }
  }
}

void alarmHandler(int sig_num) {
  
}

