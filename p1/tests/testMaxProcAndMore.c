#include "phase1.h"
#include <assert.h>
#include <stdio.h>

int Child(void *arg) {
    return 0;
}

int spotFilled(void *arg) {
    USLOSS_Console("P2_startup's spot should now be filled by the process Spot filled\n");
    P1_DumpProcesses();
    return(0);
}

int ChildNext(void *arg) {
    USLOSS_Console("1 spot should be open now that P2_StartUp has finished\n");
    P1_DumpProcesses();
    int pid = P1_Fork("Spot filled", spotFilled, NULL, USLOSS_MIN_STACK, 1, 0);
    if(pid < 0){
      USLOSS_Console("Failed to reuse PCB blocks, test failed (should be able to spawn a new process after a higher priority process finishes)\n");
      return(1);
    }
    USLOSS_Console("Passed test where more than 50 processes were created and blocks were reused in PCB table\n");
    return 0;
}


int P2_Startup(void *notused) 
{
    int pid;
    int status = 0;

    USLOSS_Console("Attempting to fill rest of table except for 1 slot\n");
    int i;
    for(i = 0; i <47; i++){
      pid = P1_Fork("Child", Child, NULL, USLOSS_MIN_STACK, 3, 0);
    }
    USLOSS_Console("Filling last slot with highest priority process\n");
    pid = P1_Fork("Filler", ChildNext, NULL, USLOSS_MIN_STACK, 1, 0);
    USLOSS_Console("All PCB spots should be filled\n");
    P1_DumpProcesses();
    return status;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {
    // Do nothing.
}
