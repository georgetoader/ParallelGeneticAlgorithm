#ifndef THREAD_DATA_H
#define THREAD_DATA_H

typedef struct _thread_data {
    int thread_id;
    int N;
    int P;
    int object_count;
    int sack_capacity;
    int *check;
    sack_object *objects;
    individual *current_generation;
    individual *next_generation;
    pthread_barrier_t *barrier;
    pthread_mutex_t *mutex;
} thread_data;

#endif