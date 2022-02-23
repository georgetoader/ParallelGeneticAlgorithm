#ifndef GENETIC_ALGORITHM_H
#define GENETIC_ALGORITHM_H

#include "sack_object.h"
#include "individual.h"
#include "thread_data.h"

int min(int a, int b);
int max(int a, int b);

// reads input from a given file
int read_input(int argc, char *argv[], sack_object **objects, int *object_count,
                 int *sack_capacity, int *generations_count, int *thread_count);

// displays all the objects that can be placed in the sack
void print_objects(const sack_object *objects, int object_count);

// displays all or a part of the individuals in a generation
void print_generation(const individual *generation, int limit);

// displays the individual with the best fitness in a generation
void print_best_fitness(const individual *generation);

// function used for creating threads
void *thread_function(void *args);

void reset_check(thread_data *tdata, int k);

void first_generation(individual *current_generation, individual *next_generation, int object_count);

void parallel_fitness_function(sack_object *objects, individual *generation, int sack_capacity, int start, int end);

void first_30_elite(thread_data *tdata, int count);

// performs a variant of bit string mutation
void mutate_bit_string_1(const individual *ind, int generation_index);

void parallel_mbs1(thread_data *tdata, int cursor, int index, int count);

// performs a different variant of bit string mutation
void mutate_bit_string_2(const individual *ind, int generation_index);

void parallel_mbs2(thread_data *tdata, int cursor, int index, int count);

// compares two individuals by fitness and then number of objects in the sack (to be used with qsort)
int cmpfunc(const void *a, const void *b);

// performs one-point crossover
void crossover(individual *parent1, individual *child1, int generatio_index);

void merge_sort(individual *generation, int left, int right);

// copies one individual
void copy_individual(const individual *from, const individual *to);

void parallel_merge_sort(individual *generation, thread_data *tdata);

void merge(individual *individual, int left, int middle, int right);

void merge_array(individual *generation, thread_data *tdata, int number, int group_cluster);

// deallocates a generation
void free_generation(individual *generation);

#endif