#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "genetic_algorithm.h"

int main(int argc, char *argv[]) {
    int N, P, object_count = 0, sack_capacity;
    sack_object *objects = NULL;

    if (!read_input(argc, argv, &objects, &object_count, &sack_capacity, &N, &P)) {
        return 0;
    }

    individual *current_generation = (individual *) calloc(object_count, sizeof(individual));
    individual *next_generation = (individual *) calloc(object_count, sizeof(individual));
    thread_data *tdata = (thread_data *) calloc(P, sizeof(thread_data));
    int *check = (int*)calloc(N, sizeof(int));

    pthread_t thread_pool[P];
    pthread_barrier_t barrier;
    pthread_mutex_t mutex;
    pthread_barrier_init(&barrier, NULL, P);
    pthread_mutex_init(&mutex, NULL);

    first_generation(current_generation, next_generation, object_count);

    for (int i = 0; i < P; ++i) {
        tdata[i].thread_id = i;
        tdata[i].N = N;
        tdata[i].P = P;
        tdata[i].object_count = object_count;
        tdata[i].sack_capacity = sack_capacity;
        tdata[i].objects = objects;
        tdata[i].check = check;
        tdata[i].current_generation = current_generation;
        tdata[i].next_generation = next_generation;
        tdata[i].barrier = &barrier;
        tdata[i].mutex = &mutex;
        pthread_create(&thread_pool[i], NULL, thread_function, &tdata[i]);
    }

    for (int i = 0; i < P; ++i) {
        pthread_join(thread_pool[i], NULL);
    }
    print_best_fitness(tdata->current_generation);

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);

    // free resources for old generation
    free_generation(current_generation);
    free_generation(next_generation);
    // free resources
    free(current_generation);
    free(next_generation);
    free(objects);
    free(tdata);
    free(check);

    return 0;
}

void *thread_function(void *args) {
    int cursor, count, group_cluster = 1;
    thread_data *tdata = (thread_data *) args;
    individual *tmp = NULL;
    int start = tdata->thread_id * (double) tdata->object_count / tdata->P;
    int end = min((tdata->thread_id + 1) * (double) tdata->object_count / tdata->P, tdata->object_count);

    for (int k = 0; k < tdata->N; ++k) {
        cursor = 0;
        // compute parallel fitness function
        parallel_fitness_function(tdata->objects, tdata->current_generation, tdata->sack_capacity, start, end);
        pthread_barrier_wait(tdata->barrier);
        // parallel mergesort
        parallel_merge_sort(tdata->current_generation, tdata);
        pthread_barrier_wait(tdata->barrier);

        // merge the sorted parts of the array
        pthread_mutex_lock(tdata->mutex);
        if (tdata->check[k] == 0) {
            merge_array(tdata->current_generation, tdata, tdata->P, group_cluster);
            tdata->check[k] = 1;
        }
        pthread_mutex_unlock(tdata->mutex);
        reset_check(tdata, k);

        // keep first 30% children (elite children selection)
        count = tdata->object_count * 3 / 10;
        first_30_elite(tdata, count);
        cursor = count;
        pthread_barrier_wait(tdata->barrier);

        // mutate first 20% children with the first version of bit string mutation
        count = tdata->object_count * 2 / 10;
        parallel_mbs1(tdata, cursor, k, count);
        cursor += count;
        pthread_barrier_wait(tdata->barrier);

        // mutate next 20% children with the second version of bit string mutation
        count = tdata->object_count * 2 / 10;
        parallel_mbs2(tdata, cursor, k, count);
        cursor += count;
        pthread_barrier_wait(tdata->barrier);

        // crossover first 30% parents with one-point crossover
        // (if there is an odd number of parents, the last one is kept as such)
        count = tdata->object_count * 3 / 10;
        pthread_mutex_lock(tdata->mutex);
        if (tdata->check[k] == 0) {
            if (count % 2 == 1) {
                copy_individual(tdata->current_generation + tdata->object_count - 1, tdata->next_generation + cursor + count - 1);
                count--;
            }
            for (int i = 0; i < count; i += 2) {
                crossover(tdata->current_generation + i, tdata->next_generation + cursor + i, k);
            }
            tdata->check[k] = 1;
        }
        pthread_mutex_unlock(tdata->mutex);
        reset_check(tdata, k);

        // switch to new generation
        tmp = tdata->current_generation;
        tdata->current_generation = tdata->next_generation;
        tdata->next_generation = tmp;
        pthread_barrier_wait(tdata->barrier);

        // parallel update index for current generation
        for (int i = start; i < end; i++) {
            tdata->current_generation[i].index = i;
        }
        pthread_barrier_wait(tdata->barrier);

        pthread_mutex_lock(tdata->mutex);
        // print best fitness value
        if (tdata->check[k] == 0) {
            // best fitness value can only pe printed once per generation
            if (k % 5 == 0) {
                print_best_fitness(tdata->current_generation);
            }
            tdata->check[k] = 1;
        }
        pthread_mutex_unlock(tdata->mutex);
        reset_check(tdata, k);
    }

    // final fitness
    parallel_fitness_function(tdata->objects, tdata->current_generation, tdata->sack_capacity, start, end);
    pthread_barrier_wait(tdata->barrier);
    // final sort
    parallel_merge_sort(tdata->current_generation, tdata);
    pthread_barrier_wait(tdata->barrier);

    pthread_mutex_lock(tdata->mutex);
    if (tdata->check[0] == 0) {
        merge_array(tdata->current_generation, tdata, tdata->P, group_cluster);
        tdata->check[0] = 1;
    }
    pthread_mutex_unlock(tdata->mutex);
    pthread_barrier_wait(tdata->barrier);

    return NULL;
}

void reset_check(thread_data *tdata, int k) {
    pthread_barrier_wait(tdata->barrier);
    tdata->check[k] = 0;
    pthread_barrier_wait(tdata->barrier);
}
