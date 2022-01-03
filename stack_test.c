#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define n 3
int limit = -1;
int* stack_limit = &limit;
int stack_limit_arr[n] = {0};


struct Stack {
    int* top;
    unsigned capacity;
    int** array;
};


// function to create a stack of given capacity. It initializes size of
// stack as 0
struct Stack* createStack(unsigned capacity, unsigned size_perm)
{
    struct Stack* stack = (struct Stack*)malloc(sizeof(struct Stack));
    int** perm_array = malloc(stack->capacity * sizeof(int)*size_perm);
    stack->capacity = capacity;
    stack->top = stack_limit;
    stack->array = perm_array;
    return stack;
}
 
// Stack is full when top is equal to the last index
int isFull(struct Stack* stack)
{
    printf("isFull: %d\n", *stack->top);
    printf("isFull: %d\n", stack->capacity-1);
    fflush(stdout);  

    return *stack->top == stack->capacity - 1;
}
 
// Stack is empty when top is equal to -1
int isEmpty(struct Stack* stack)
{
    return *stack->top == -1;
}
 
// Function to add an item to stack.  It increases top by 1
void push(struct Stack* stack, int* item)
{    
    if (isFull(stack))
        return;
    *stack->top += 1;
    printf("%d", *stack->top);
    fflush(stdout);  

    stack->array[*stack->top] = item;

    printf("Pushed: [%d", item[0]);
    for(int i=1;i<3;i++){
        printf(", %d", item[i]);
    }
    printf("] \n");

}
 
// Function to remove an item from stack.  It decreases top by 1
int* pop(struct Stack* stack)
{
    if (isEmpty(stack))
        return stack_limit;
    int * to_return = stack->array[*stack->top];
    *stack->top += -1;
    return to_return;
    

}
 
// Function to return the top from stack without removing it
int* peek(struct Stack* stack)
{
    if (isEmpty(stack))
        return stack_limit;
    return stack->array[*stack->top];
}

void free_stack(struct Stack* stack){
    free(stack->array);
    free(stack);

}

int main()
{   
    struct Stack* stack = createStack(100, n);
    int array1[3] = {10,10,20}; int array2[3] = {30,30,40}; int array3[3] = {50,50,60};

    push(stack, array1);
    push(stack, array2);
    push(stack, array3);



    for (int j=0;j<4;j++){
        int* popped = pop(stack);
        printf("Popped: %d", *popped);
        if (*popped != -1){
            printf("Popped: [%d", popped[0]);
            for (int i=1; i<n;i++){
                printf(", %d", popped[i]);
            }
            printf("] \n");
        }
        else{
            printf("Empty stack!");
        }

    }
    free_stack(stack);
    return 0;
}

