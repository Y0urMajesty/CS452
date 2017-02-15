#include <stdio.h>
#include "usloss.h"

USLOSS_Context OS;
USLOSS_Context hello;
USLOSS_Context world;


#define SIZE ( USLOSS_MIN_STACK * 2 )

char stackH[SIZE];
char stackW[SIZE];

void printHello( void ) {

    for( int i = 1; i <= 10; i++ ) {
        USLOSS_Console( "%d Hello ", i );
        USLOSS_ContextSwitch( &hello, &world );
    }
    USLOSS_Halt(0);
}

void printWorld( void ) {
    for( int j = 1; j <= 10 ; j++ ) {
        USLOSS_Console( "World" );
        for( int k = 1; k <= j ; k++ ) {
        USLOSS_Console( "!" );
    }
    USLOSS_Console( "\n" );
    USLOSS_ContextSwitch( &world, &hello );
    }
}

void initializeMe( void ) {
    USLOSS_ContextInit( &hello, stackH, sizeof( stackH ), NULL, printHello );
    USLOSS_ContextInit( &world, stackW, sizeof( stackW ), NULL, printWorld );
    USLOSS_ContextSwitch( &OS, &hello );

}

void startup( int argc, char **argv ) {
    initializeMe();
}

void finish( int argc, char **argv ) {
}

void test_setup( int argc, char **argv ) {}

void test_cleanup( int argc, char **argv ) {}
