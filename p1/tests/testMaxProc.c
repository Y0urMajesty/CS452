#include "phase1.h"
#include <assert.h>
#include <stdio.h>

int Child(void *arg) {
    return 0;
}

int P2_Startup(void *notused) 
{
    int pid;
    int status = 0;

    int i;
    for(i = 0; i <50; i++){
      pid = P1_Fork("Child", Child, NULL, USLOSS_MIN_STACK, 3, 0);
    }
    if(pid != -1){
      USLOSS_Console("Fork returned %d, wrong value when more than 50 processes created\n", pid);
      return(1);
    }
    P1_DumpProcesses();
    USLOSS_Console("Passed test where more than 50 processes were created. Please scan your output from P1_DumpProcesses for error checking\n");
    return status;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {
    // Do nothing.
}
