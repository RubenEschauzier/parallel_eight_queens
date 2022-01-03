#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int n = 8;
int board[8][8];
int num_evals = 0;
int num_sol = 0;

struct matrix {
  unsigned nrows, ncolumns;
  int values[];
};

struct matrix* create_matrix(int mnrows, int mncolumns) {
    size_t sz = sizeof(struct matrix)+(mnrows*mncolumns*sizeof(int));
    struct matrix *m = malloc(sz);

    if (!m) { 
        perror("malloc mymatrix"); exit(EXIT_FAILURE); 
    }

    m->nrows = mnrows;
    m->ncolumns = mncolumns;
    for (int i = mnrows * mncolumns-1; i>=0; i--){
        m->values[i] = i;
    }

    return m;

} /*end create_matrix*/

static inline int getmatrix(struct matrix *m, int row, int col) {
  if (!m) {
     fprintf(stderr, "getmatrix with no matrix\n");
     exit(EXIT_FAILURE);
  }
  if (row >= m->nrows || col >= m->ncolumns || row < 0 || col < 0){
     fprintf(stderr, "getmatrix out of bounds\n");
     exit(EXIT_FAILURE);
  }
  return m->values[row * m->ncolumns + col];
}

struct matrix* generate_board(int n){
    // Remeber to free array
    struct matrix *board = create_matrix(n, n);
    for (int i = 0; i < n; ++i){
        for (int j = 0; j < n; ++j){
            printf("%d \n", getmatrix(board, i, j));
        }
    }
    return board;
}

int attempt_queen_placement(int row){
    int obstructed;
    if (row >= n || row < 0){
        fprintf(stderr, "Error: Row out of bounds\n");
        return -1;
    }

    for (int i = 0; i < n; i++){
        obstructed = is_obstructed(n, row, i);

        if (obstructed == 0){
            board[row][i] = 1;
            return 1;
        }
    }

    return 0;
}

// Use heaps algorithm to create permutations
int permutation_based(int k, int perm[]){
    int temp_swap;

    if (k==1){
        return 1;
    }
    else{

        permutation_based(k-1, perm);

        for (int i = 0; i < k-1; i+=1){
            num_evals += 1;

            if (k & 1){
                /* k is odd */
                swap_index(perm, 0, k-1);            }
            else{
                /* k is even */
                swap_index(perm, i, k-1);
            }
            for (int j = 0; j < 4; j++){
                printf("%d ", perm[j]);
            }


            printf("\n");
            permutation_based(k-1, perm);
        }
    }

}

void recursive_search(int perm[], int base[], int size_p){

    for (int i = 0; i < size_p; i++){
        printf("Perm: ");
        for (int z = 0; z < size_p; z++){
            printf("%d ", perm[z]);
        }
        printf("\n");
        memcpy(perm, base, sizeof(perm[0])*size_p);
        generate_branch_queens_test(perm, i, size_p);
    }
}


void generate_branch(int perm[], int k, int size_p){
    printf("New function call: k=%d \n", k);
    for (int i = k + 1; i < size_p; i++){
        int *to_swap = malloc(size_p*sizeof(int));
        memcpy(to_swap, perm, size_p*sizeof(int));

        swap_index(to_swap, i, k);

        num_evals += 1;
        for (int z = 0; z < size_p; z++){
            printf("%d ", to_swap[z]);
        }
        printf("\n");


        if (k < size_p - 1){
            for (int j = k + 1; j < size_p; j++){
            generate_branch(to_swap, j, size_p);
            }
        }
        free(to_swap);
    }

}

void generate_branch_queens(int perm[], int k, int size_p){
    for (int z = 0; z < size_p; z++){
        printf("%d ", perm[z]);
    }
    printf(" k = %d\n",k);

    if (k == size_p-1){   
        if (is_attacked(perm, k) == 0){
            printf("solution!! Checked %d\n", k);
            num_sol += 1;
            // for (int z = 0; z < size_p; z++){
            //     printf("%d ", perm[z]);
            // }
            // printf("\n");
        }
        return;
    }


    for (int i = k + 1; i < size_p; i++){
        int *to_swap = malloc(size_p*sizeof(int));
        memcpy(to_swap, perm, size_p*sizeof(int));

        swap_index(to_swap, i, k);

        num_evals += 1;



        if (k < size_p - 1 && is_attacked(to_swap, k) == 0){
            for (int j = k + 1; j < size_p; j++){

                printf("Called recursion: k = %d \nCalled by: ", j);
                for (int z = 0; z < size_p; z++){
                    printf("%d ", perm[z]);
                }
                printf("\n");
                
                generate_branch_queens(to_swap, j, size_p);
            }
        }
        free(to_swap);
    }
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
