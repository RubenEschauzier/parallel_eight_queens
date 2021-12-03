#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <bsp.h>

#define MAX_N 20
#define MAX_P 128

static unsigned int P;
static unsigned int n;
unsigned long num_sol;
unsigned long num_evals;
int bool_true = 1;
int bool_false = 0;
int request_work[2*MAX_P];
int new_work_input[MAX_P*(MAX_N+1)];
int finished_sending[MAX_P];
int input_array[MAX_N];


int fact(int i){
    /* factorial function */
	if (i <= 1) return 1;
  	else return i*fact(i-1);
}

void swap_index(int perm[], int i, int j){
    /*Swap array values at given indices*/
    int temp_swap = perm[i];
    perm[i] = perm[j];
    perm[j] = temp_swap;
}

int is_attacked(int perm[], int k){
    /* Function to check for diagonal attacks */
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

void generate_branch_queens(unsigned long* solutions, int perm[], int k, unsigned int size_p){
    /*  Index of given node is index += size_p - (k+1) * index in loop
        If else statements are kinda bogus, do distribution of nodes in the bsp part and do only further search here*/
        num_evals += 1;

    if (k == size_p-1){   
        if (is_attacked(perm, k) == 0){
            num_sol += 1;
            *solutions += 1;
        }
        return;
    }

    
    for (int i = k; i < size_p; i++){
        // Malloc a new permutation so we only edit permutations within our branch
        int *to_swap = malloc(size_p*sizeof(int));
        memcpy(to_swap, perm, size_p*sizeof(int));
        swap_index(to_swap, i, k);

        // If the queen is placed safely we keep doing recursions
        if (is_attacked(to_swap, k) == 0){
            generate_branch_queens(solutions, to_swap, k+1, size_p);
        }
        // After all recursion we can free our malloc
        free(to_swap);
    }
    
}

void generate_branch_load_balance(int s, int perm[], int k, unsigned int size_p){
    /* Index of given node is index += size_p - (k+1) * index in loop
    If else statements are kinda bogus, do distribution of nodes in the bsp part and do only further search here*/
    num_evals += 1;
    int* n_messages;
    long* bytes_recieved;

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
            if (size_p - k > 3){
                // Loop to check if a processor needs any work, if so we put the current permutation and
                // Depth into work variable and continue on another node
                for (int j=0;j<P;j++){
                    if(request_work[2*j] && request_work[(2*j)+1] == s){
                        for (int z=0; z<n; z++){
                            bsp_hpput(j, &to_swap, new_work_input, (j*(n+1)+z)*sizeof(int), sizeof(int));
                        }
                        // We signal current depth, this means the reciever must call function with k+1
                        bsp_hpput(j, &k, new_work_input, (j*(n+1)+n)*sizeof(int), sizeof(int));
                        bsp_hpput(j, &bool_true, finished_sending, j*sizeof(int), sizeof(int));
                        bsp_sync();
                    }
                }
            }
            // bsp_qsize(n_messages, bytes_recieved);
            // if (n_messages > 0){
            //     printf("P%d Got Mail \n", s);
            // }
            generate_branch_load_balance(s, to_swap, k+1, size_p);
        }
        free(to_swap);
    }
    
}


void divide_work(unsigned long* solutions, unsigned long* work_load, long proc_num, 
                 int dist_depth, int node_index, int perm[], int k, 
                 unsigned int size_p, int load_balance){
    /*We divide the work between processors by calculating the indices of the nodes at desired distribution
    Depth and then calling the queen search to processors in accordance with the cyclic distribution*/
    if (k < dist_depth){
        for (int j = k; j < size_p; j++){
            int *to_swap = malloc(size_p*sizeof(int));
            memcpy(to_swap, perm, size_p*sizeof(int));
            swap_index(to_swap, j, k);

            if (is_attacked(to_swap, k) == 0){
                // Function to get index of 'leaf' nodes that need to be divided unto processors
                int new_node_index = node_index + j * fact(size_p - (size_p - k));
                divide_work(solutions, work_load, proc_num, dist_depth, new_node_index, to_swap, k+1, size_p, load_balance);
            }
            free(to_swap);
        }
    }
    if (k == dist_depth){
        //printf("Node index: %d ", node_index);
        // Cyclic distribution
        if (proc_num == 0){
            // printf("Node Index: %d \n", node_index);
        }

        if (node_index % P == proc_num){
            if (!load_balance){                
                *work_load += 1;
                generate_branch_queens(solutions, perm, k, size_p);
            }
            else{
                generate_branch_load_balance(proc_num, perm, k, size_p);
            }
        }
    }
}

void spmd_search_unbalanced() {
    /* Search all solutions in an unbalanced matter, meaning we do not care if one processor is done as
    is not doing anyting. This eliminates any communication */
    bsp_begin(P);
    long s = bsp_pid();
    unsigned long work_load = 0;
    unsigned long solutions = 0;
    unsigned long solutions_per_p[P];
    bsp_push_reg(solutions_per_p, P*sizeof(long));
    bsp_sync();

    // Make sure we have enough branches to divide 
    int to_divide = n;
    int it_depth = 1;
    while (P > to_divide){
        // Each layer of depth represents a part of the n! number of permutations
        to_divide = to_divide * n - it_depth;
        it_depth += 1;
    }
    // Start the main part by dividing the work between processors
    divide_work(&solutions, &work_load, s, it_depth, 0, input_array, 0, n, 0);
    bsp_sync();

    // Broadcast the number of solutions each processor found
    for (int i=0;i<P;i++){
        bsp_put(i, &solutions, solutions_per_p, sizeof(long)*s, sizeof(long));
    }
    bsp_sync();

    // We add up all solutions found by the processors
    unsigned long total_solutions = 0;
    for (int j=0;j<P;j++){
        total_solutions += solutions_per_p[j];
    }
    if (s == 0){
        printf("We found %ld solutions. \ncd ", total_solutions);
    }

    //printf("P%ld: %ld  \n", s, work_load);
    bsp_end();
}

void spmd_search_load_balanced(){
    /*Search all solutions while making sure that when a processor is done he gets another node to work
    from*/
    // Use send instead of put! This seems to do what we want
    bsp_begin(P);

    srand(time(NULL)); 
    long s = bsp_pid();   
    unsigned long solutions = 0;
    unsigned long work_load = 0;




    int total_branches[P];
    int new_work_input[P*(n+1)];
    int finished_sending[P];
    int p_proposition = -1;

    // Make sure we have enough branches to divide 
    int to_divide = n;
    int it_depth = 1;
    while (P > to_divide){
        // Each layer of depth represents a part of the n! number of permutations
        to_divide = to_divide * n - it_depth;
        it_depth += 1;
    }
    int branches = to_divide % P;
    // Push the required variables so we can signal when a processor needs work
    bsp_push_reg(request_work, 2*P*sizeof(int));
    bsp_push_reg(new_work_input, P*(n+1)*sizeof(int));
    bsp_push_reg(finished_sending, P*sizeof(int));
    bsp_sync();


    divide_work(&solutions, &work_load, s, it_depth, 0, input_array, 0, n, 1);
    // When a processor is done we immediataly push a request for more work by using high performance put
    // We randomly assign a processor that is still working for our current processor to request from
    int all_done = 0;
    // while (!all_done){
        
    //     int proc = rand() % P;
    //     if (!request_work[2*proc] && proc != s){
    //         p_proposition = proc;
    //     }
    //     if (p_proposition != -1){
    //         printf("First to send: P%ld to P%d\n", s, p_proposition);
    //         bsp_hpsend(p_proposition, &bool_true, &s, sizeof(int));
    //     }
    //}
    while (!all_done){
        // Pick a random processor
        int proc = rand() % P;
        // Check if the processor is not already requesting work
        if(!request_work[2*proc]){
            p_proposition = proc;
        }
        // If there exists a processor that has work we get it
        if(p_proposition != -1){
            // Put out request for work
            bsp_hpput(s, &bool_true, request_work, 2*s*sizeof(int), sizeof(int));
            bsp_hpput(s, &p_proposition, request_work, ((2*s)+1)*sizeof(int), sizeof(int));
            bsp_sync();
            printf("P %ld: ", s);
            for (int j = 0; j<P*2; j++){
                printf("%d ", request_work[j]);
            }
            printf("\n");



            // Keep looping untill processor p_proposition has sent us work or processor p_proposition needs
            // work themselves
            while (!request_work[p_proposition]){
                // printf("P%ld: ", s);
                // for (int j = 0; j<n; j++){
                //     printf("%d ", request_work[j]);
                // }
                // printf("\n");

                // If the processor has finished sending us work, we call recursion on given work
                if(finished_sending[s] && request_work[s]){
                    int new_permutation[n];
                    for (int i=0;i<n;i++){
                        new_permutation[i] = new_work_input[s*(n+1)+i];
                    } 
                    generate_branch_load_balance(s, new_permutation, new_work_input[s*(n+1)+n], n);
                }
            }
        }
        all_done = 1;
        for (int i=0;i<P;i++){
            if (request_work[2*i] == 0){
                all_done = 0;
            }
        }
        p_proposition = -1;
    }
    bsp_pop_reg(&request_work);
    bsp_pop_reg(&new_work_input);
    bsp_pop_reg(&finished_sending);
    // If we find a processor we can request from we perform our request, else we just end program since 
    // we're the last working processor
    bsp_sync();
    bsp_end();
}
void run_experiment(int num_runs, int argc, char**argv){
    struct timespec begin, end;
    double timings[num_runs];

    for (int i = 0; i < num_runs; ++i){
        clock_gettime(CLOCK_MONOTONIC_RAW, &begin);

        bsp_init(&spmd_search_unbalanced, argc, argv );
        spmd_search_unbalanced();

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
write

int main( int argc, char ** argv ) {
    if (atoi(argv[1]) > MAX_N || atoi(argv[2]) > MAX_P){
        printf("Exceeded MAX_N or MAX_P. \n");
        return EXIT_FAILURE;
    }
    n = atoi(argv[1]);
    P = atoi(argv[2]); 

    for (int i = 0; i < n; i++){
        input_array[i] = i;
    }
    // for (int i = 1; i < 16; i++){
    //     P = i;
    //     printf("Running Experiment %d: \n", P);
    //     run_experiment(1000, argc, argv);
    // }
    //run_experiment(1000, argc, argv);
    bsp_init(&spmd_search_unbalanced, argc, argv);
    spmd_search_unbalanced();
    //printf("Solutions found: %ld \n", num_sol);

    return EXIT_SUCCESS;
}

