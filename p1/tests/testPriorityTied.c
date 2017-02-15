#include "phase1.h"
#include <assert.h>
#include <stdio.h>

int Child1(void *arg) {
    USLOSS_Console("Child #5\n");
    return 0;
}

int Child2(void *arg) {
    USLOSS_Console("Child #1\n");
    return 0;
}

int Child3(void *arg) {
    USLOSS_Console("Child #2\n");
    return 0;
}

int Child4(void *arg) {
    USLOSS_Console("Child #3\n");
    return 0;
}

int Child5(void *arg) {
    USLOSS_Console("Child #4\n");
    return 0;
}

int P2_Startup(void *notused) 
{
    int pid;
    int status = 0;

    USLOSS_Console("P2_Startup\n");
    USLOSS_Console("Each child should spawn in order from 1 to 5 (equal priority test)\n");
    pid = P1_Fork("Child", Child1, NULL, USLOSS_MIN_STACK, 5, 0);
    pid = P1_Fork("Child", Child2, NULL, USLOSS_MIN_STACK, 2, 0);
    pid = P1_Fork("Child", Child3, NULL, USLOSS_MIN_STACK, 2, 0);
    pid = P1_Fork("Child", Child4, NULL, USLOSS_MIN_STACK, 2, 0);
    pid = P1_Fork("Child", Child5, NULL, USLOSS_MIN_STACK, 2, 0);
    return status;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {
    // Do nothing.
}
