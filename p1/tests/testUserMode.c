#include "phase1.h"

int P2_Startup(void *notused) 
{
    USLOSS_Console("Turning on User mode!\n");
    int s = USLOSS_PsrSet((USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_MODE) | USLOSS_PSR_CURRENT_INT);
    USLOSS_Console("Current Mode %d\n", s );

    return 0;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {
    // Do nothing.
}
