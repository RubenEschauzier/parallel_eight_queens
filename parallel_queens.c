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

struct Queue {
    int front, rear, size;
    int capacity;
    int** array;
};

struct Queue* createQueue(int capacity, int size_perm)
{

    struct Queue* queue = (struct Queue*)malloc(
        sizeof(struct Queue));

    queue->capacity = capacity;

    queue->front = 0;
    queue->size = 0;
    queue->rear = capacity - 1;

    queue->array = malloc(queue->capacity * sizeof(int)*size_perm);;
    return queue;
}
 
// Queue is full when size becomes
// equal to the capacity
int isFull(struct Queue* queue)
{
    return (queue->size == queue->capacity);
}
 
// Queue is empty when size is 0
int isEmpty(struct Queue* queue)
{
    return (queue->size == 0);
}
 
// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue* queue, int* item)
{

    if (isFull(queue))
        return;

    queue->rear = (queue->rear + 1)
                  % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size += 1;

}
 
// Function to remove an item from queue.
// It changes front and size
int* dequeue(struct Queue* queue)
{
    if (isEmpty(queue)){
        return NULL;
    }
    int* item = queue->array[queue->front];

    queue->front = (queue->front + 1)
                   % queue->capacity;
    queue->size += - 1;
    return item;
}



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
void write_to_file(double timings[], int max_p){
    FILE *f = fopen("log/nqueens_all", "a");
    fprintf(f, "n");
    for (int i=1;i<max_p+1;i++){
        fprintf(f, ", %d",i);
    }
    fprintf(f, "\n%d", n);
    for (int j=0;j<max_p;j++){
        fprintf(f, ", %f", timings[j]);
    }
    fprintf(f, "\n");
    fclose(f);
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
void divide_work_queue(unsigned long* solutions, unsigned long* work_load, long proc_num, 
                       int dist_depth, int perm[], unsigned int size_p){

    struct Queue* queue = createQueue(500, size_p);
    int division_depth = 0;
    for(int j=division_depth;j<size_p;j++){

        int *to_swap = malloc(size_p*sizeof(int));
        memcpy(to_swap, perm, size_p*sizeof(int));
        swap_index(to_swap, j, division_depth);
        if (is_attacked(to_swap, division_depth) == 0){
            enqueue(queue, to_swap);
        }
    }
    division_depth += 1;
    for (int i=1; i<dist_depth;i++){

        int current_q_size = queue->size;
        for(int l=0; l<current_q_size;l++){

            int* stack_perm = dequeue(queue);
            for(int z = division_depth; z<size_p;z++){
                int *to_swap = malloc(size_p*sizeof(int));
                memcpy(to_swap, stack_perm, size_p*sizeof(int));
                swap_index(to_swap, z, division_depth);

                if (is_attacked(to_swap, division_depth) == 0){
                    enqueue(queue, to_swap);
                }
            }
        }
        division_depth += 1;
    }
    int branch_division_counter = 0;
    int max_q_size = queue->size;
    for(int q = 0; q<max_q_size; q++){
        int* work = dequeue(queue);
        // printf("Deq'd: [%d", work[0]);
        //     for (int i=1; i<n;i++){
        //         printf(", %d", work[i]);
        //     }
        // printf("] \n");
        //printf("P%ld, Dequeued: %d, division_counter: %d \n", proc_num, work[0], branch_division_counter);

        if (branch_division_counter % P == proc_num){
            //printf("P%ld, Div_c %d\n", proc_num, branch_division_counter);
            *work_load += 1;
            generate_branch_queens(solutions, work, division_depth, size_p);
        }
        branch_division_counter += 1;

    }
    free(queue);
}
void spmd_search_queue(){
    bsp_begin(P);
    long s = bsp_pid();
    unsigned long work_load = 0;
    unsigned long solutions = 0;
    unsigned long solutions_per_p[P];
    bsp_push_reg(solutions_per_p, P*sizeof(long));
    bsp_sync();

    int to_divide = n;
    int it_depth = 1;
    while (P > to_divide){
        // Each layer of depth represents a part of the n! number of permutations
        to_divide = to_divide * n - it_depth;
        it_depth += 1;
    }
    // Add one to required depth to ensure more even distribution of work
    it_depth += 1;
    // For testing purposes
    divide_work_queue(&solutions, &work_load, s, it_depth, input_array, n);

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
        printf("We found %ld solutions. \n", total_solutions);
    }

    printf("P%ld: %ld  \n", s, work_load);
    bsp_pop_reg(solutions_per_p);
    bsp_sync();
    bsp_end();
 
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
        printf("We found %ld solutions. \n", total_solutions);
    }

    printf("P%ld: %ld  \n", s, work_load);
    bsp_pop_reg(solutions_per_p);
    bsp_sync();
    bsp_end();
}

double run_experiment(int n_runs, int argc, char**argv){
    struct timespec begin, end;
    double timings[n_runs];

    for (int i = 0; i < n_runs; ++i){
        clock_gettime(CLOCK_MONOTONIC_RAW, &begin);

        bsp_init(&spmd_search_unbalanced, argc, argv );
        spmd_search_unbalanced();

        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
         
        timings[i] = (end.tv_nsec - begin.tv_nsec) / 1000000000.0 + (end.tv_sec  - begin.tv_sec);
    }
    
    double average_timing = 0;
    for (int j = 0; j < n_runs; ++j){
        average_timing += timings[j];
    }
    average_timing /= n_runs;
    printf("Average Time: %f \n", average_timing);
    return average_timing;
}

void run_multiple_experiment(int n_runs, int max_p, int argc, char**argv){
    double experiment_timings[max_p-1];
    for (int i=1;i<max_p+1;i++){
        P = i;
        experiment_timings[i-1] = run_experiment(n_runs, argc, argv);
    }
    write_to_file(experiment_timings, max_p);

}

int main( int argc, char ** argv ) {
    // https://stackoverflow.com/questions/10479446/what-should-i-do-to-get-the-whole-return-value-of-c-program-from-command-line
    if (atoi(argv[1]) > MAX_N || atoi(argv[2]) > MAX_P){
        printf("Exceeded MAX_N or MAX_P. \n");
        return EXIT_FAILURE;
    }
    n = atoi(argv[1]);
    P = atoi(argv[2]); 

    for (int i = 0; i < n; i++){
        input_array[i] = i;
    }
    //run_multiple_experiment(100, atoi(argv[2]), argc, argv);
    // for (int i = 1; i < 16; i++){
    //     P = i;
    //     printf("Running Experiment %d: \n", P);
    //     run_experiment(1000, argc, argv);
    // }
    //run_experiment(1000, argc, argv);
    bsp_init(&spmd_search_queue, argc, argv);
    spmd_search_queue();

    return EXIT_SUCCESS;
}

