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
