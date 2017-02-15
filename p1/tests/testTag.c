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

    pid = P1_Fork("Child", Child, NULL, USLOSS_MIN_STACK, 3, -999);
    if(pid != -4){
      USLOSS_Console("Error, fork should have returned -4 due to invalid tag, but fork returned %d\n", pid);
    } else{
      USLOSS_Console("Passed test where incorrect tag was given as a parameter to fork\n");
    }
    return status;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {
    // Do nothing.
}
