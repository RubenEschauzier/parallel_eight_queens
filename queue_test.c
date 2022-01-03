#include <limits.h>
#include <stdio.h>
#include <stdlib.h>


int zero_s = 0;
int zero_f = 0;
int limit = -1;
int rear_test = 99;

int* start_size = &zero_s;
int* start_front = &zero_f;
int* queue_limit = &limit;


struct Queue {
    int front, rear, size;
    int capacity;
    int** array;
};

struct Queue* createQueue(int capacity, int size_perm)
{

    struct Queue* queue = (struct Queue*)malloc(
        sizeof(struct Queue));

    int** perm_array = malloc(queue->capacity * sizeof(int)*size_perm);

    queue->capacity = capacity;

    queue->front = 0;
    queue->size = 0;
    queue->rear = capacity - 1;

    queue->array = perm_array;
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

int main()
{
    int n = 3;

    struct Queue* queue = createQueue(100, n);
    int array1[3] = {10,10,20}; int array2[3] = {30,30,40}; int array3[3] = {50,50,60};
 
    enqueue(queue, array1);
    enqueue(queue, array2);
    int* returned = dequeue(queue);
    int* returned1 = dequeue(queue);

    return EXIT_SUCCESS;   
}
