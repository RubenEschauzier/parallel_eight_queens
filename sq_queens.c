#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Optimization ideas: Each half done solution can be flipped over y axis to generate another solution
                      Use bitmaps to denote if a queen is hittin square or not

https://codereview.stackexchange.com/questions/196509/parallel-solutions-to-n-queens-puzzle (good stackoverflow3)
https://www.geeksforgeeks.org/printing-solutions-n-queen-problem/ (implementation of bitmap)
Use performance profiles, Geometric means in report
*/

int num_sol = 0;
int n = 8;
int board[8][8];
long num_evals;


void print_board(){
    printf("     ");
    for (int k = 0; k < n; k++){
        printf(" %d  ", k);
    }
    printf("\n \n");
    for (int i = 0; i < n; i++){
        printf("%d   |", i);
        for (int j = 0; j < n; j++){
            printf(" %d |", board[i][j]);
        }
        printf("\n");
    }

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

void gbq_test(int perm[], int k, int size_p){
    num_evals += 1;

    
    for (int i = k+1; i < size_p; i++){
        // Create new array to swap to ensure that in future recursions the starting array is correct
        int *to_swap = malloc(size_p*sizeof(int));
        memcpy(to_swap, perm, size_p*sizeof(int));

        swap_index(to_swap, i, k);
        printf("Permutations: \n");
        for (int z = 0; z < size_p; z++){
            printf("%d ", to_swap[z]);
        }
        printf("\n");
        printf("K = %d \n", k);
        printf("Attacked: %d \n", is_attacked(to_swap, k));

        if (is_attacked(to_swap, k) == 0){
            for (int j = i+1; j<size_p; j++){
                gbq_test(to_swap, j, size_p);
            }
        }

        free(to_swap);
    }
}

void generate_branch_queens_test(int perm[], int k, int size_p){
    num_evals += 1;

    if (k == size_p-1){   
        if (is_attacked(perm, k) == 0){
            printf("solution!! Checked %d\n", k);
            num_sol += 1;
            for (int z = 0; z < size_p; z++){
                printf("%d ", perm[z]);
            }
            printf("\n");
        }
        return;
    }
    else{
        for (int i = k; i < size_p; i++){

            int *to_swap = malloc(size_p*sizeof(int));
            memcpy(to_swap, perm, size_p*sizeof(int));
            swap_index(to_swap, i, k);

            if (is_attacked(to_swap, k) == 0){

                printf("Called recursion: k = %d \nCalled by: ", k+1);
                for (int z = 0; z < size_p; z++){
                    printf("%d ", perm[z]);
                }
                printf("\n");

                generate_branch_queens_test(to_swap, k+1, size_p);
            }
            free(to_swap);
        }
    }
}


// def explore(ntries, n, perm, index):
//    if index == n - 1:
//       if is_safe(perm, index):
//          yield perm
//       return
//    # create all single swaps at index with equal or greater indices
//    for i in range(index, n):
//       perm_i = list(perm)
//       perm_i[index], perm_i[i] = perm[i], perm[index]
//       next(ntries)
//       # check the diagonals for all smaller indices
//       if is_safe(perm_i, index):
//          # call the recursion with the next index
//          for p in explore(ntries, n, perm_i, index+1):
//             yield p


void recursive_search(int perm[], int base[], int size_p){
    for (int i = 0; i < size_p; i++){
        memcpy(perm, base, sizeof(perm[0])*size_p);
        generate_branch_queens_test(perm, i, size_p);
    }
}


int is_obstructed(int n, int row, int col){
    // Function to check obstructions, assumes there is no queen on 'higher' rows and that there are no attempts to place queen on same row.
    // Can be optimized!!!

    for (int i = row-1; i >= 0; i--){
        if (board[i][col] == 1){
            return 1;
        }
    }
    for (int r = row, c = col; r >= 0 && c < n; r--, c++){
        if (board[r][c] == 1){
            return 1;
        }
    }
    for (int rd = row, cd = col; rd >= 0 && cd >= 0; rd--, cd--){
        if(board[rd][cd] == 1){
            return 1;
        }
    }
    return 0;
}


int attempt_place_queen(int row, int col){
    /* Try to place a queen without it being attacked, returns 1 if sucessful.
     0 if it is not succesful, queen won't be placed. */

    /* Check if the attempt to place queen is not out of bounds*/
    if (row >= n || row < 0 || col >= n || col < 0){
        fprintf(stderr, "Error: Row out of bounds\n");
        return -1;
    }

    /* Check if queen is attacked, returns 0 if it is not attacked*/
    int obs = is_obstructed(n, row, col);

    /* Place queen if not attacked*/
    if (obs == 0){
        board[row][col] = 1;
        return 1;
    }
    return 0;
}

int fill_out_board(int row, int col){
    int fill_row;
    int succes = attempt_place_queen(row, col);

    num_evals += 1;
    if (succes == 0){
        return -1;
    }

    if (succes == 1 && row == n-1){
        num_sol += 1;
        return 1;
    }
    for (int i = 0; i < n; i++){
        fill_row = fill_out_board(row+1, i);
        if (fill_row == -1){
            continue;
        }
        if (fill_row == 1){
            board[row+1][i] = 0;
        }
    }

    return 1;
}

void backtracking_solver(){
    int num_found;

    for (int i = 0; i < n; i++){
        num_found = fill_out_board(0, i);
        board[0][i] = 0;
    }
}


int main( int argc, char ** argv ) {
    int n = 8;
    memset(board, 0, n*n*sizeof(int));
    
    // backtracking_solver();
    // printf("Num sols: %d \n", num_sol);
    int test[5] = {0,1,2,3,4};
    int base[5] = {0,1,2,3,4};
    int size_p = sizeof(test)/sizeof(test[0]);
    recursive_search(test, base, size_p);
    printf("Num evals %ld \n", num_evals);
    printf("Num sol: %d", num_sol);

}
