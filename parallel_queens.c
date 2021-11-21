#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bsp.h>


static unsigned int P;
static unsigned int n;

void spmd_search() {
    bsp_begin(P);
    if (P > n){
        //Assign work to n^2 cyclic
    }
    else{
        //Assign work cyclic
    }
    printf( "Hello world from thread %u out of %u!\n", bsp_pid(), bsp_nprocs() );
    bsp_end();
}

int main( int argc, char ** argv ) {
    P = 4;
    n = 4;
    int board_state[n];
    for (int i = 0; i < n; i++){
        board_state[i] = i;
    }

    bsp_init( &spmd_search, argc, argv );
    spmd_search();
    return EXIT_SUCCESS;
}

