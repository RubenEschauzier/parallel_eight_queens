#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <bsp.h>


static unsigned int P;
static unsigned int n;
unsigned long num_sol;
unsigned long num_evals;
int input_array[14] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13};


int fact(int i){
	if (i <= 1) return 1;
  	else return i*fact(i-1);
}

void swap_index(int perm[], int i, int j){
    int temp_swap = perm[i];
    perm[i] = perm[j];
    perm[j] = temp_swap;
}

int is_attacked(int perm[], int k){
    int dL_index = perm[k] + k;
    int dR_index = perm[k] - k;

    for (int i = 0; i < k; i++){
        int dL_i = perm[i] + i;
        int dR_i = perm[i] - i;
        if (dL_index == dL_i || dR_index == dR_i){
            return 1;
        }
    }
    return 0;
}

void generate_branch_queens(long proc_num, int dist_depth, int node_index, int perm[], int k, unsigned int size_p){
    // Index of given node is index += size_p - (k+1) * index in loop
    // If else statements are kinda bogus, do distribution of nodes in the bsp part and do only further search here
    num_evals += 1;

    if (k == size_p-1){   
        if (is_attacked(perm, k) == 0){
            num_sol += 1;
        }
        return;
    }

    
    for (int i = k; i < size_p; i++){

        int *to_swap = malloc(size_p*sizeof(int));
        memcpy(to_swap, perm, size_p*sizeof(int));
        swap_index(to_swap, i, k);

        if (is_attacked(to_swap, k) == 0){
            generate_branch_queens(proc_num, dist_depth, node_index, to_swap, k+1, size_p);
        }
        free(to_swap);
    }
    
}

void divide_work(long proc_num, int dist_depth, int node_index, int perm[], int k, unsigned int size_p){
    if (k < dist_depth){
        for (int j = k; j < size_p; j++){
            int *to_swap = malloc(size_p*sizeof(int));
            memcpy(to_swap, perm, size_p*sizeof(int));
            swap_index(to_swap, j, k);

            if (is_attacked(to_swap, k) == 0){
                // Function to get index of 'leaf' nodes that need to be divided unto processors
                int new_node_index = node_index + j * fact(size_p - (k+1));
                divide_work(proc_num, dist_depth, new_node_index, to_swap, k+1, size_p);
            }
            free(to_swap);
        }
    }
    if (k == dist_depth){
        // Cyclic distribution
        if (node_index % P == proc_num){
            // Check this, this is where it possibly goes wrong!
            generate_branch_queens(proc_num, dist_depth, node_index, perm, k, size_p);
        }
    }
}

void spmd_search() {
    bsp_begin(P);
    long s = bsp_pid();
    // Make sure we have enough branches to divide 
    int to_divide = n;
    int it_depth = 1;
    while (P > to_divide){
        // Each layer of depth represents a part of the n! number of permutations
        to_divide = to_divide * n - it_depth;
        it_depth += 1;
    }
    printf("Iteration depth = %d", it_depth);
    // Assign work using cyclic distribution
    divide_work(s, it_depth, 0, input_array, 0, n);
    
    bsp_end();
}
void run_experiment(int num_runs, int argc, char**argv){
    struct timespec begin, end;
    double timings[num_runs];

    for (int i = 0; i < num_runs; ++i){
        clock_gettime(CLOCK_MONOTONIC_RAW, &begin);

        bsp_init(&spmd_search, argc, argv );
        spmd_search();

        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
         
        timings[i] = (end.tv_nsec - begin.tv_nsec) / 1000000000.0 + (end.tv_sec  - begin.tv_sec);
    }
    
    double average_timing = 0;
    for (int j = 0; j < num_runs; ++j){
        average_timing += timings[j];
    }
    average_timing /= num_runs;
    printf("Average Time: %f \n", average_timing);
}

int main( int argc, char ** argv ) {
    long max_p = 8;
    P = 1;
    n = 14;
    int board_state[n];
    for (int i = 0; i < n; i++){
        board_state[i] = i;
    }

    // run_experiment(10000, argc, argv);
    for (int i = 1; i < max_p; i++){
        P = i;
        run_experiment(100, argc, argv);
    }
    // bsp_init( &spmd_search, argc, argv );
    // spmd_search();
    printf("Solutions found: %ld \n", num_sol);

    return EXIT_SUCCESS;
}

