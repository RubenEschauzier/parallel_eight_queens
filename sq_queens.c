#include <stdio.h>
#include <stdlib.h>

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

int main( int argc, char ** argv ) {
    int board_size = 8;
    struct matrix* board = generate_board(board_size);

}
