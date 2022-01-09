#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <bsp.h>
#include <mcbsp.h>

#define MAX_N 40
#define MAX_P 128

static unsigned int P;
static unsigned int n;
unsigned long num_sol;
unsigned long num_evals;
int input_array[MAX_N];
int solution[MAX_N];


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

void free_queue(struct Queue* queue){
    free(queue->array);
    free(queue);
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


void generate_branch_queens(unsigned long* solutions, int perm[], int k, unsigned int size_p, long proc_num){
    mcbsp_internal_check_aborted();

    /*  Index of given node is index += size_p - (k+1) * index in loop
        If else statements are kinda bogus, do distribution of nodes in the bsp part and do only further search here*/
        num_evals += 1;

    if (k == size_p-1){   
        if (is_attacked(perm, k) == 0){
            num_sol += 1;
            *solutions += 1;
            printf("P%ld Found a solution! \n", proc_num);
            printf("Solution: [%d", perm[0]);
            for(int i=1;i<n;i++){
                printf(", %d", perm[i]);
            }
            printf("] \n");
            fflush(stdout);
            abort();
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
            generate_branch_queens(solutions, to_swap, k+1, size_p, proc_num);
        }
        // After all recursion we can free our malloc
        free(to_swap);
    }
    
}
void divide_work_queue(unsigned long* solutions, unsigned long* work_load, long proc_num, 
                       int dist_depth, int perm[], unsigned int size_p, int max_qu_size){

    struct Queue* queue = createQueue(1000, size_p);
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
        if (branch_division_counter % P == proc_num){
            *work_load += 1;
            printf("P%ld is starting work on ", proc_num);
            fflush(stdout);
            generate_branch_queens(solutions, work, division_depth, size_p, proc_num);
        }
        branch_division_counter += 1;
        free(work);

    }
    free_queue(queue);
}


void divide_work_queue_random(unsigned long* solutions, unsigned long* work_load, long proc_num, int dist_depth, int perm[], 
unsigned int size_p, int max_queue_size, int random_division[]){
    srand(time(NULL));
    struct Queue* queue = createQueue(1000, size_p);
    int division_depth = 0;

    /*Do first iteration of breath first search without dequeueing to initialise the search*/
    for(int j=division_depth;j<size_p;j++){
        int *to_swap = malloc(size_p*sizeof(int));
        memcpy(to_swap, perm, size_p*sizeof(int));
        swap_index(to_swap, j, division_depth);

        if (is_attacked(to_swap, division_depth) == 0){
            enqueue(queue, to_swap);
        }
    }

    division_depth += 1;
    /* Iteration based breadth first search untill required distribution depth*/
    for (int i=1; i<dist_depth;i++){
        /*Iterate over all nodes put in queue by previous depth*/
        int current_q_size = queue->size;
        for(int l=0; l<current_q_size;l++){
            /*Get next node from queue*/
            int* stack_perm = dequeue(queue);
            for(int z = division_depth; z<size_p;z++){
                /*Generate all new nodes based on dequeued node*/
                int *to_swap = malloc(size_p*sizeof(int));
                memcpy(to_swap, stack_perm, size_p*sizeof(int));
                swap_index(to_swap, z, division_depth);
                /*If this node is valid (queen placed not attacked) we put in queue for next iteration or work division*/
                if (is_attacked(to_swap, division_depth) == 0){
                    enqueue(queue, to_swap);
                }
            }
            free(stack_perm);
        }
        division_depth += 1;
    }
    /* Here we create pseudo random assignment of branches on processor 0, it's pseudo_random due to the requirement of around even distribution of branches.
    Furthermore, the rand() function is also pseudorandom. We could remove the communication and synchronisation costs by performing this operation on all 
    processors by generating the same random number sequence on each processor. However, the rand() function is not threadsafe and thus doesn't produce equal
    random number sequences. This is a hacky solution that slightly slows the algorithm, however it shouldn't be substantial.
     */
    int max_q_size = queue->size;
    if (proc_num == 0){
        /*Keep track of number of assigned brances and the max branches assigned, this max branches isn't optimized, especially for large # processors*/
        int num_assigned_branches[MAX_P] = {0};
        int max_assignment = (int)ceil((double)queue->size / P);
        for(int q = 0; q<max_q_size; q++){
            int assign_success = 0;
            /*Keep trying to assign untill success*/
            while (assign_success == 0){
                int assignment= (rand() % (P-1 - 0 + 1)) + 0;
                if (num_assigned_branches[assignment] < max_assignment){
                    /*If we succeed, do some bookkeeping*/
                    assign_success = 1;
                    num_assigned_branches[assignment] += 1;
                    random_division[q] = assignment;
                    }

                }
            }
        /*Communicate the division made by processor 0*/
        for (int proc = 0; proc<P; proc++){
            for(int k=0;k<max_q_size;k++){
                bsp_put(proc, &random_division[k], random_division, sizeof(int)*k, sizeof(int));
            }
        }
    }
    bsp_sync();
    /*Now all processors remove all branches from queue and check if that branch is assigned to them, if so they start working*/
    for(int q = 0; q<max_q_size; q++){
        int* work = dequeue(queue);
        if (random_division[q] == proc_num){
            *work_load += 1;
            generate_branch_queens(solutions, work, division_depth, size_p, proc_num);
        }
        free(work);
    }
    free_queue(queue);
}


void spmd_search_queue(){
    bsp_begin(P);
    /*Initialise the random number generator to ensure that all processors generate the same random numbers*/

    long s = bsp_pid();
    unsigned long work_load = 0;
    unsigned long solutions = 0;
    unsigned long solutions_per_p[P];

    int to_divide = n;
    int it_depth = 1;
    while (P > to_divide){
        // Each layer of depth represents a part of the n! number of permutations
        to_divide = to_divide * n - it_depth;
        it_depth += 1;
    }
    // Add one to required depth to ensure more even distribution of work
    int max_queue_size =fact(n)-fact(n-it_depth);
    int random_division[1000];

    bsp_push_reg(solutions_per_p, P*sizeof(long));
    bsp_push_reg(random_division, max_queue_size*sizeof(int));
    bsp_sync();
    divide_work_queue(&solutions, &work_load, s, it_depth, input_array, n, max_queue_size);

    /* Broadcast the number of solutions each processor found */
    for (int i=0;i<P;i++){
        bsp_put(i, &solutions, solutions_per_p, sizeof(long)*s, sizeof(long));
    }
    bsp_sync();

    // We add up all solutions found by the processors
    unsigned long total_solutions = 0;
    for (int j=0;j<P;j++){
        total_solutions += solutions_per_p[j];
    }
    // if (s == 0){
    //     printf("We found %ld solutions. \n", total_solutions);
    // }

    // printf("P%ld: %ld  \n", s, work_load);
    bsp_pop_reg(solutions_per_p);
    bsp_pop_reg(random_division);
    bsp_sync();
    bsp_end();
 
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
    bsp_init(&spmd_search_queue, argc, argv);
    spmd_search_queue();

    return EXIT_SUCCESS;
}
