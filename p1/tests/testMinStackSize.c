#include "phase1.h"
#include <assert.h>
#include <stdio.h>

int Child(void *arg) {
    USLOSS_Console("Child\n");
    return 0;
}

int P2_Startup(void *notused) 
{
    int pid;
    int status = 0;

    pid = P1_Fork("Child", Child, NULL, 1, 3, 0);
    if(pid != -2){
      USLOSS_Console("Error, fork should have returned -2 due to too small of a stack size, but fork returned %d\n", pid);
    } else {
      USLOSS_Console("Passed test where an invalid stack size was given\n");
    }

    return status;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {
    // Do nothing.
}
