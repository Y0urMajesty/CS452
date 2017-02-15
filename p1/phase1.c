/* ------------------------------------------------------------------------
 CSC452
 Assignment: phase1.c (PART B)
 Authors: Dana Sandy and Dylan Kuper
 Date: 2/21/17
 
 Description: Phase 1 provides basic process support to the USLOSS OS, using semaphores, join and interrupts.
 ------------------------------------------------------------------------ */
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "usloss.h"
#include "phase1.h"

#define SUCCESS                 0

// Processes states
#define INVALID_PID             -1
#define RUNNING                 0
#define READY                   1
#define EMPTY                   2
#define QUIT                    3
#define WAITING                 4

// Fork error return values
#define NO_PROCESSES            -1
#define SMALL_STACK             -2
#define INVALID_PRIORITY        -3
#define INVALID_TAG             -4
#define MAX_PRIORITY            6
#define ILLEGAL_INSTRUCTION     100
#define ORPHAN                  -99

#define NO_STATUS               -99
#define NO_TAG                  -99
#define NO_TAG_MATCH            -1

#define SEM_WAIT                1

// SemCreate errors
#define WAIT_ON_SEM             -2
#define INVALID_SEM             -1
#define SEM_FULL                -2
#define SEM_DUP                 -1



/* -------------------------- Globals ------------------------------------- */

typedef struct PCB {
    USLOSS_Context 	 context;
    int        		 (*startFunc)(void *);   /* Starting function */
    void        		 *startArg;    		 /* Arg to starting function */
    
    // For bookkeeping
    int        		 pid;
    int        		 parent;
    int        		 priority;
    int        		 state;
    int        		 status;
    int        		 tag;
    int        		 numChild;
    int        		 startTime;
    int        		 stopTime;
    int        		 totalTime;
    void       		 *stack;
    char       		 *processName;
    char       		 *semaphoreName;
} PCB;


// Queue for each type of Semaphore
typedef struct Sem_Queue {
    int             	sem_pid;
    struct Sem_Queue	*q_next;
} Sem_Queue;

// Semaphore
typedef struct Semaphore{
    char       		 *semaphoreName;
    int        		 count;
    int        		 waiting;
    Sem_Queue       	*q_head;
    Sem_Queue   		*q_tail;
} Semaphore;


// DMS: I think we need to make a few lists, like ready, waiting or at the least a semaphore array that carries the semaphores created.
P1_Semaphore semaphoreList[P1_MAXSEM]; //void * [1000]

/* the process table */
PCB procTable[P1_MAXPROC];


/* current process ID */
int pid = -1;

/* number of processes */
int numProcs = 0;


// FUNCTION PROTOTYPES
void Check_Mode(void);
void clock_handler(int type, void *arg);
void dispatcher(void);
void P1_DumpProcesses(void);
int findNextProcess(int);
void finish(int argc, char **argv);
int P1_Fork(char *name, int (*f)(void *), void *arg, int stacksize, int priority, int tag);
static void launch(void);
int P1_GetPID(void);
int P1_GetState(int PID);
int P1_GetTag(void);
void illegalinstruction_handler( int type, void *arg );
void P1_Quit( int status );
static int sentinel(void *arg);
void startup(int argc, char **argv);
char* state2String( int state );


// FUNCTIONS IN ALPHA ORDER, P1 PREFIX IGNORED
/* -------------------------- Functions ----------------------------------- */




/* -----------------------------------------------------------------------
 Name - Check_Mode
 Purpose - checks if in kernel mode
 Parameters - none
 Returns - nothing
 Side Effects - None, yet. If in user mode, prints an exception message.
 
 -----------------------------------------------------------------------*/
void Check_Mode() {
    
    int kernel = USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE;
    if( !kernel ) {
        USLOSS_IllegalInstruction();
    }
    
}

/* ------------------------------------------------------------------------
 Name - dispatcher
 Purpose - runs the highest priority runnable process
 Parameters - none
 Returns - nothing
 Side Effects - runs a process
 PART B: The quantum used for time-slicing is 80 milliseconds (four clock ticks). New processes should be placed at the end of the list of processes with the same priority.
 ----------------------------------------------------------------------- */
void dispatcher() {
    /*
     * Run the highest priority runnable process. There is guaranteed to be one
     * because the sentinel is always runnable.
     */
    
    Check_Mode();
    
    int oldpid = pid;
    for( int priority = 1; priority <= MAX_PRIORITY; priority++ ) {
        for( int process = 0; process < P1_MAXPROC; process++ ) {
            if( procTable[process].state == READY && procTable[process].priority == priority ) {
                pid = process;
                if( procTable[oldpid].state == RUNNING ) {
                    procTable[oldpid].state = READY;
                    
                    // Collecting CPU time from when process was paused
                    if( USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &procTable[oldpid].stopTime ) == USLOSS_DEV_INVALID ) {
                        USLOSS_Console( "Clock Device Invalid!\n" );
                    }
                    procTable[oldpid].totalTime = procTable[oldpid].totalTime + (procTable[oldpid].stopTime - procTable[oldpid].startTime);
                    procTable[oldpid].startTime = 0;
                    procTable[oldpid].stopTime = 0;
                    
                }
                procTable[process].state = RUNNING;
                
                // Starting the new process's clock
                if( USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &procTable[process].startTime ) == USLOSS_DEV_INVALID ) {
                    USLOSS_Console( "Clock Device Invalid!\n" );
                }
                USLOSS_ContextSwitch(&procTable[oldpid].context,&procTable[process].context);
                return;
            }
        }
    }
}

/*This routine should print process information to the console for debugging purposes.
 For each PCB in the process table print (at a minimum) its PID, parent’s PID, priority, process state (e.g. unused, running, ready, waiting, etc.),
 # of children, CPU time consumed, name, and name of the semaphore on which it is waiting (if any).
 No particular format is necessary, but make it look nice.
 */
void P1_DumpProcesses(void) {
    for( int i = 0; i < P1_MAXPROC; i++ ) {
        if( procTable[i].state == EMPTY ) {
            USLOSS_Console( "Process Name: None\n" );
            USLOSS_Console( "PID: %d\n", procTable[i].pid );
            USLOSS_Console( "Parent PID: None\n", procTable[i].parent );
            USLOSS_Console( "Priority: None\n" );
            USLOSS_Console( "Process state: %s\n", state2String( procTable[i].state ));
            USLOSS_Console( "Number of Children: None\n" );
            USLOSS_Console( "CPU Time Consumed: None\n\n" );
        }
        else {
            USLOSS_Console( "Process Name: %s\n", procTable[i].processName );
            USLOSS_Console( "PID: %d\n", procTable[i].pid );
            if( procTable[i].parent == ORPHAN ) {
                USLOSS_Console( "Parent PID: None\n" );
            }
            else {
                USLOSS_Console( "Parent PID: %d\n", procTable[i].parent );
            }
            USLOSS_Console( "Priority: %d\n", procTable[i].priority );
            USLOSS_Console( "Process state: %s\n", state2String( procTable[i].state ));
            USLOSS_Console( "Number of Children: %d\n", procTable[i].numChild );
            USLOSS_Console( "CPU Time Consumed: %d\n\n", procTable[i].totalTime );
        }
    }
}


/* -----------------------------------------------------------------------
 Name - findNextProcess
 Purpose - finds next process with the specified passed in state
 Parameters - state
 Returns - index of process with the passed in state, -1 otherwise
 Side Effects - none
 
 -----------------------------------------------------------------------*/
int findNextProcess( int state ) {
    
    for( int i = 0; i < P1_MAXPROC; i++ ) {
        if( procTable[i].state == state ) {
            return i;
        }
    }
    return NO_PROCESSES;
}

/* ------------------------------------------------------------------------
 Name - finish
 Purpose - Required by USLOSS
 Parameters - none
 Returns - nothing
 Side Effects - none
 ----------------------------------------------------------------------- */
void finish(int argc, char **argv) {
    USLOSS_Console("Goodbye.\n");
} /* End of finish */


/* ------------------------------------------------------------------------
 Name - P1_Fork
 Purpose - Gets a new process from the process table and initializes
 information of the process.  Updates information in the
 parent process to reflect this child process creation.
 Parameters - the process procedure address, the size of the stack and
 the priority to be assigned to the child process.
 Returns - the process id of the created child or an error code.
 Side Effects - ReadyList is changed, procTable is changed, Current
 process information changed
 ------------------------------------------------------------------------ */
int P1_Fork(char *name, int (*f)(void *), void *arg, int stacksize, int priority, int tag) {
    
    // Make sure it's in kernel mode
    Check_Mode();
    
    // Check validity of tag
    if( priority < 1 || priority > MAX_PRIORITY ) {
        return INVALID_PRIORITY;
    }
    
    // Check validity of tag
    if( tag != 0 && tag != 1 ) {
        return INVALID_TAG;
    }
    
    // Check validity of stack size
    if( stacksize < USLOSS_MIN_STACK ) {
        return SMALL_STACK;
    }
    
    // Looking for the next unused process (looking for EMPTY, for part B)
    int newPid = findNextProcess( EMPTY );
    
    if( newPid == NO_PROCESSES ) {
        return NO_PROCESSES;
    }
    
    // Initialize new PCB in procTable at newPid index
    procTable[newPid].startFunc = f;
    procTable[newPid].startArg = arg;
    
    // allocate stack used for context
    procTable[newPid].stack = malloc( stacksize );
    // initialize context
    USLOSS_ContextInit( &procTable[newPid].context, procTable[newPid].stack, stacksize, P3_AllocatePageTable(newPid), launch );
    
    // set parent to current process
    if( pid == -1 && priority != 6 ) {
        procTable[newPid].parent = 0;
        procTable[0].numChild++;
    }
    else {
        procTable[newPid].parent = pid;
        procTable[pid].numChild++;
    }
    
    
    // allocate process name string
    procTable[newPid].processName = (char*) calloc( strlen(name) + 1, sizeof(char) );
    
    // copy name of process
    strcpy(procTable[newPid].processName, name);
    
    // set priority
    procTable[newPid].priority = priority;
    // set state to READY
    procTable[newPid].state = READY;
    
    // Initializing clock
    procTable[newPid].startTime = 0;
    procTable[newPid].stopTime = 0;
    
    if( USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &procTable[newPid].startTime) == USLOSS_DEV_INVALID ) {
        USLOSS_Console("Clock Device Invalid Error.\n");
    }
    
    procTable[newPid].totalTime = 0;
    
    procTable[newPid].tag = tag;
    
    // increment global process counter
    numProcs++;
    
    // check if process should continue with current priority
    // check if child process is higher priority than parent
    if( procTable[pid].priority > procTable[newPid].priority ) {
        dispatcher();
    }
    return newPid;
} /* End of fork */


/* ------------------------------------------------------------------------
 Name - P1_GetPID
 Purpose - gets the PID of the process
 Parameters - None
 Returns - current process PID
 Side Effects - none
 ------------------------------------------------------------------------ */
int P1_GetPID(void) {
    return pid;
}


/* ------------------------------------------------------------------------
 Name - P1_GetState
 Purpose - gets the state of the process
 Parameters - process PID
 Returns - process state
 -1: invalid PID
 0: the process is running
 1: the process is ready
 2: (not used)
 3: the process has quit
 4: the process is waiting on a semaphore
 Side Effects - none
 ------------------------------------------------------------------------ */
int P1_GetState(int PID) {
    return procTable[PID].state;
}

int P1_GetTag(void){
    return procTable[pid].tag;
}


/*-------------------------------------------------------------------------
 PART B:
 This operation synchronizes termination of a child with its parent.
 The tag specifies the tag of the child to be waited for.
 When a parent process calls P1_Join waits for one of its children with a matching tag to exit children exits (hint: use a semaphore).
 P1_Join returns immediately if a child has already exited and has not already been joined.
 P1_Join returns the PID of the child process that exited and stores in  status the status parameter the child passed to P1_Quit.
 Note: the tag functionality will be used in Phase 2 to allow user-level processes to wait for user-level processes
 and kernel-level processes to wait for kernel-level processes, respectively.
 Return values:
 -1: the process doesn’t have any children with a matching tag
 >= 0: the PID of the child that quit
 0: success
 ------------------------------------------------------------------------*/
int P1_Join( int tag, int *status ) {
    
    // parent waits in join for child with matching tag to quit
    int parentPID = pid;
    int childPID = -1;
    
    //if( procTable[parentPID].numChild > 0 ) {
    for( int i = 0; i < P1_MAXPROC; i++ ) {
        if( procTable[i].parent == parentPID && procTable[i].tag == tag ) {
            childPID = procTable[i].pid;
            if( procTable[childPID].state == QUIT){ //If child has already quit, return child's PID
                *status = procTable[childPID].status;
                procTable[childPID].state = EMPTY;
                return childPID;
            }
        }
    }
    if(childPID == -1 ){ //Has children, but no matching tag
        return NO_TAG_MATCH;
    }
    //}
    // else{ //Has no children
    return -1;
    //}
    
    //BEWARE: SEMAPHORES BELOW.
    
    return SUCCESS;
    
}


/* ------------------------------------------------------------------------
 Name - launch
 Purpose - Dummy function to enable interrupts and launch a given process
 upon startup.
 Parameters - none
 Returns - nothing
 Side Effects - enable interrupts
 ------------------------------------------------------------------------ */
void launch(void) {
    Check_Mode();
    
    int  rc;
    int  status;
    status = USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
    if (status != 0) {
        USLOSS_Console("USLOSS_PsrSet failed: %d\n", status);
        USLOSS_Halt(1);
    }
    rc = procTable[pid].startFunc(procTable[pid].startArg);
    
    /* quit if we ever come back */
    P1_Quit(rc);
} /* End of launch */


/* ------------------------------------------------------------------------
 Name - P1_Quit
 Purpose - Causes the process to quit and wait for its parent to call P1_Join.
 Parameters - quit status
 Returns - nothing
 Side Effects - the currently running process quits
 PART B:
 Another hint: the child must wait (hint: use a semaphore) until its parent calls P1_Join so that the parent can get the status from the child.
 ------------------------------------------------------------------------ */
void P1_Quit( int status ) {
    
    Check_Mode();
    
    // Bookkeeping before you quit a process. Undoing what's done in the fork.
    if( USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &procTable[pid].stopTime ) == USLOSS_DEV_INVALID ) {
        USLOSS_Console( "Clock Device Invalid!\n" );
    }
    procTable[pid].totalTime = procTable[pid].totalTime + (procTable[pid].stopTime - procTable[pid].startTime);
    
    // Free process name string
    free( procTable[pid].processName );
    
    // Free page table
    P3_FreePageTable(pid);
    
    // Update parents and children information
    // if current process is not an orphan, decrement parent's number of children
    int parent = procTable[pid].parent;
    if( parent >= 0 ) {
        procTable[parent].numChild--;
    }
    
    // if current process has children, set children as orphans
    int children = procTable[pid].numChild;
    if( children > 0 ) {
        for( int c = 0; c < P1_MAXPROC; c++ ) {
            if( procTable[c].parent == pid ) {
                procTable[c].parent = ORPHAN;
            }
        }
    }
    
    // Free context stack
    free( procTable[pid].stack );
    
    // If the process is an orphan, free the block for re-use
    if(procTable[pid].parent == ORPHAN){
        procTable[pid].state = EMPTY;
    }
    // If a parent may expect the process to quit, set state as QUIT
    else{
        procTable[pid].state = QUIT;
        procTable[pid].status = status;
    }
    // decrement number of processes
    numProcs--;
    
    dispatcher();
}


/* ------------------------------------------------------------------------
 PART B:
 Return the CPU time (in microseconds) used by the current process.
 This means that the kernel must record the amount of processor time used by each process.
 Do not use the clock interrupt to measure CPU time as it is too coarse-grained;
 read the time from the USLOSS clock device instead.
 ----------------------------------------------------------------------- */

int P1_ReadTime( void ) {
    // I believe the stuff that I wrote to calculate the CPU time should go in here.
    int stopTime = 0;
    if( USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &stopTime ) == USLOSS_DEV_INVALID ) {
        USLOSS_Console( "Clock Device Invalid!\n" );
    }
    else {
        return procTable[pid].totalTime += stopTime - procTable[pid].startTime;
    }
    
    return 0;
}


/***************************SEMAPHORES*********************************/

int int_enable(){
    USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
}

int int_disable(){
    int retval = USLOSS_PsrGet() & USLOSS_PSR_CURRENT_INT;//getbit here USLOSS_PSR_CURRENT_INT
    USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT);
    return retval;
}

/* ------------------------------------------------------------------------
 This operation creates a new semaphore named name with its initial value set to value.
 and returns a pointer it in sem. You may assume a maximum of P1_MAXSEM semaphores.
 Return values:
 -2: P1_MAXSEM semaphores already exist.
 -1: duplicate semaphore name
 0: success
 ------------------------------------------------------------------------ */
int P1_SemCreate( char *name, unsigned int value, P1_Semaphore *sem ) {
    Semaphore * realsem;
    for(int i = 0; i < P1_MAXSEM; i++){
        realsem = (Semaphore*)semaphoreList[i];
        if(realsem != NULL){
            if(strcmp(realsem->semaphoreName, name) != 0){
                return SEM_DUP;
            }
        }
        if(i == P1_MAXSEM - 1 && realsem != NULL){
            return SEM_FULL;
        }
    }
    
    sem = malloc(sizeof(Semaphore));
    if(sem == NULL){
        USLOSS_Console("MALLOC ERROR: allocating semaphore\n");
    }
    realsem = (Semaphore*)sem;
    realsem->name = (char*)malloc(strlen(name)+1);
    if(realsem->name == NULL){
        USLOSS_Console("MALLOC ERROR: alocating semaphore name\n");
    }
    strcpy(realsem->name, name);
    realsem->value = value;
    realsem->waiting = 0;
    realsem->q_head = NULL;
    realsem->q_tail = NULL;
    return SUCCESS;
}

/* ------------------------------------------------------------------------
 Free the indicated semaphore. If there are processes waiting on the semaphore then an error message is printed and USLOSS_Halt(1) is called.
 Return values:
 -2: processes are waiting on the semaphore
 -1: the semaphore is invalid
 0: success
 ------------------------------------------------------------------------ */
int P1_SemFree( P1_Semaphore sem ) {
    Semaphore *realsem = (Semaphore*)(sem);
    if(realsem == NULL){
        return INVALID_SEM;
    }
    if(realsem->waiting){
        USLOSS_Console("ERROR: A Semaphore with a waiting process is trying to be freed\n");
        USLOSS_Halt(1);
        return WAIT_ON_SEM;
    }
    free(realsem->name);
    free(realsem);
    
    return SUCCESS;
    
}

/* ------------------------------------------------------------------------
 Perform a P operation on the indicated semaphore.
 Return values:
 -2: the process was killed
 -1: the semaphore is invalid
 0: success
 ------------------------------------------------------------------------ */
int P1_P( P1_Semaphore sem ) {
    Semaphore *realsem = (Semaphore *)(sem);
    
    int enabled;
    while(1) {
        enabled = int_disable();
        if (realsem->count > 0) {
            realsem->count--;
            goto done;
        }
        realsem->waiting = SEM_WAIT;
        // Move process from ready queue to s->q (NEED TO DO THIS)
        Sem_Queue *newtail = (Sem_Queue*)malloc(sizof(Sem_Queue));
        newtail->sem_pid = pid;
        //INSERT NODE INTO END OF QUEUE
        
        
        //SET PROCESS STATUS TO WAITING
        
        
        
        if (enabled) {
            int_enable();
        }
        dispatcher();
    }
done:
    if (enabled) {
        realsem->waiting = !SEM_WAIT;
        int_enable();
    }
    return SUCCESS;
    
}

/* ------------------------------------------------------------------------
 Perform a V operation on the indicated semaphore.
 Return values:
 -1: the semaphore is invalid
 0: success
 ------------------------------------------------------------------------ */
int P1_V( P1_Semaphore sem ) {
    Semaphore * realsem = (Semaphore*)(sem);
    int enabled;
    enabled = int_disable();
    realsem->count++;
    if (realsem->q_head != NULL) {
        //DEQUEUE FIRST PROCESS FROM QUEUE AND SET ITS PROCESS STATE TO READY
        
        
        if (enabled) {
            int_enable();
        }
        dispatcher();
    }
    
    return SUCCESS;
    
}

/* ------------------------------------------------------------------------
 Returns the name of the semaphore, NULL if sem is invalid.
 ------------------------------------------------------------------------ */
char * P1_GetName( P1_Semaphore sem ) {
    Semaphore * realsem = (Semaphore*)(sem);
    if(realsem == NULL){
        return NULL;
    }
    if(realsem.name == NULL){
        return NULL;
    }
    return realsem.name;
}

/**********************************************************************/


/* ------------------------------------------------------------------------
 Name - sentinel
 Purpose - The purpose of the sentinel routine is two-fold.
 One responsibility is to keep the system going when all other
 processes are blocked. The other is to detect and report
 simple deadlock states.
 Parameters - none
 Returns - nothing
 Side Effects -  if system is in deadlock, print appropriate error
 and halt.
 ----------------------------------------------------------------------- */
int sentinel (void *notused) {
    
    while (numProcs > 1) {
        /* Check for deadlock here */
        USLOSS_WaitInt();
    }
    
    USLOSS_Halt(0);
    /* Never gets here. */
    return 0;
} /* End of sentinel */


/* ------------------------------------------------------------------------
 Name - startup
 Purpose - Initializes semaphores, process lists and interrupt vector.
 Start up sentinel process and the P2_Startup process.
 Parameters - none, called by USLOSS
 Returns - nothing
 Side Effects - lots, starts the whole thing
 ----------------------------------------------------------------------- */
void startup(int argc, char **argv) {
    
    //To handle the various interrupts, the OS must fill in the interrupt vector with the addresses of the interrupt handlers.
    //The vector declared as a global array in USLOSS ( USLOSS_IntVec ), and is simply referenced by name.
    //The symbolic constants for the devices are designed to be used as indexes when initializing the table.
    
    // Interrupts are disabled when startup is invoked, providing an opportunity to initialize the interrupt vector.
    USLOSS_IntVec[USLOSS_ALARM_INT] = alarm_handler;
    USLOSS_IntVec[USLOSS_CLOCK_INT] = clock_handler;
    USLOSS_IntVec[USLOSS_DISK_INT] = disk_handler;
    USLOSS_IntVec[USLOSS_ILLEGAL_INT] = illegalinstruction_handler;
    USLOSS_IntVec[USLOSS_MMU_INT] = memory_handler;
    USLOSS_IntVec[USLOSS_SYSCALL_INT] = syscall_handler;
    USLOSS_IntVec[USLOSS_TERM_INT] = terminal_handler;
    
    /* initialize the process table here */
    for( int i = 0; i < P1_MAXPROC; i++ ) {
        procTable[i].state = EMPTY;
        procTable[i].pid = i;
        procTable[i].status = NO_STATUS;
        procTable[i].status = NO_TAG;
    }
    /* Set all semaphores pointers in semaphoreList to NULL */  //NULL void pointer may have issues in future???
    for(int i = 0; i < P1_MAXSEM; i++){
        semaphoreList[i] = NULL;
    }
    
    Check_Mode();
    /* Initialize the Ready list, waiting list, etc. here */
    
    /* Initialize the interrupt vector here */
    
    /* Initialize the semaphores here */
    
    /* startup a sentinel process */
    /* HINT: you don't want any forked processes to run until startup is finished.
     * You'll need to do something  to prevent that from happening.
     * Otherwise your sentinel will start running as soon as you fork it and
     * it will call P1_Halt because it is the only running process.
     */
    
    P1_Fork("sentinel", sentinel, NULL, USLOSS_MIN_STACK, 6, 0);
    
    /* start the P2_Startup process */
    P1_Fork("P2_Startup", P2_Startup, NULL, 4 * USLOSS_MIN_STACK, 1, 0);
    
    dispatcher();
    
    /* Should never get here (sentinel will call USLOSS_Halt) */
    
    return;
} /* End of startup */


/* ------------------------------------------------------------------------
 Name - state2String
 Purpose - Returns the string definition of the state integer passed in.
 Parameters - state
 Returns - string
 Side Effects - none
 ------------------------------------------------------------------------ */
char* state2String( int state ) {
    switch ( state ) {
        case EMPTY:
            return "EMPTY";
            break;
        case RUNNING:
            return "RUNNING";
            break;
        case READY:
            return "READY";
            break;
        case WAITING:
            return "WAITING";
            break;
        case INVALID_PID:
            return "INVALID PID";
            break;
        case QUIT:
            return "QUIT";
            break;
        default:
            break;
    }
    return "NULL";
}

/***************************INTERRUPTS*********************************/


/*
 Interrupt handlers are passed two parameters. The first parameter indicates the type of interrupt,
 allowing the same handler to handle more than one type of interrupt, if desired.
 The second parameter varies depending on the type of interrupt. Generally, an interrupt handler
 takes some action and then returns, allowing execution to resume at the point where it was interrupted.
 In some cases, however, the interrupt handler may execute a context switch, in which case the current
 machine state is saved and execution is resumed at another point. An interrupt handler that invokes a
 context switch must be carefully written, as the state of the OS after the switch will almost certainly
 be different than the state before.
 
 When an interrupt occurs, USLOSS switches to kernel mode, disables interrupts, and calls the appropriate
 routine stored in the interrupt vector. Six different types of interrupts/devices are simulated
 (symbolic constants are shown in parentheses):
 ● clock ( USLOSS_CLOCK_INT  and  USLOSS_CLOCK_DEV )
 ● count-down timer ( USLOSS_ALARM_INT  and  USLOSS_ALARM_DEV )
 ● terminal ( USLOSS_TERM_INT  and  USLOSS_TERM_DEV )
 ● system call ( USLOSS_SYSCALL_INT )
 ● disk ( USLOSS_DISK_INT  and  USLOSS_DISK_DEV )
 ● memory-management unit ( USLOSS_MMU_INT )
 ● illegal instruction ( USLOSS_ILLEGAL_INT )
 
 */

void alarm_handler( int type, void *arg ) {
    USLOSS_Console("Handling Alarm\n" );
    // Does alarm stuff
}

void clock_handler( int type, void *arg ) {
    USLOSS_Console("Handling Clock\n" );
    // Does clock stuff
}

void disk_handler( int type, void *arg ) {
    USLOSS_Console("Handling Disk\n" );
    // Does disk stuff
}

void illegalinstruction_handler( int type, void *arg ) {
    USLOSS_Console("Illegal Instruction Error, Quitting Current Process!\n" );
    P1_Quit( ILLEGAL_INSTRUCTION );
}

void memory_handler( int type, void *arg ) {
    USLOSS_Console("Handling Memory\n" );
    // Does memory stuff
}

void syscall_handler( int type, void *arg ) {
    USLOSS_Console("Handling System Call\n" );
    // Does syscall stuff
}

void terminal_handler( int type, void *arg ) {
    USLOSS_Console("Handling Terminal\n" );
    // Does terminal stuff
}


/*-------------------------------------------------------------------------
 PART B:
 Perform a P operation on the semaphore associated with the given unit of the device type.
 The device types are defined in usloss.h.
 The appropriate device semaphore is V ’ed every time an interrupt is generated by the I/O device,
 with the exception of the clock device which should only be V ’ed every 100 ms (every 5 interrupts).
 This routine will be used to synchronize with a device driver process in the next phase.
 P1_WaitDevice returns the device’s status register in * status.
 Note: the interrupt handler calls USLOSS_DeviceInput to get the device’s status.
 This is then saved until P1_WaitDevice is called and returned in * status.
 P1_WaitDevice does not call USLOSS_DeviceInput.
 Return values:
 -3: the wait was aborted via  P1_WakeupDevice
 -2: invalid type
 -1: invalid unit
 0: success
 ------------------------------------------------------------------------*/
int P1_WaitDevice( int type, int unit, int *status ) {
    return 0;
    
}

/*-------------------------------------------------------------------------
 Causes  P1_WaitDevice to return.
 If abort is non-zero then P1_WaitDevice returns that the wait was aborted (-3),
 otherwise P1_WaitDevice returns success (0).
 The interrupt handlers for the devices should call P1_WakeupDevice with abort set to zero to cause P1_WaitDevice to return.
 Return values:
 -2: invalid type
 -1: invalid unit
 0: success
 ------------------------------------------------------------------------*/
int P1_WakeupDevice( int type, int unit, int abort ) {
    return 0;
    
}

/**********************************************************************/




