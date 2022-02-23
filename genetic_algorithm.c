#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "genetic_algorithm.h"

int min(int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

int max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

int read_input(int argc, char *argv[], sack_object **objects, int *object_count,
                 int *sack_capacity, int *generations_count, int *thread_count) {
    if (argc < 4) {
        fprintf(stderr, "Usage:\n\t./tema1_par in_file generations_count thread_count\n");
        return 0;
    }

    FILE *fp;
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        return 0;
    }

    if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
        fclose(fp);
        return 0;
    }

    if (*object_count % 10) {
        fclose(fp);
        return 0;
    }

    sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

    for (int i = 0; i < *object_count; ++i) {
        if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
            free(objects);
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);

    *generations_count = atoi(argv[2]);
    *thread_count = atoi(argv[3]);

    if (*generations_count == 0) {
        free(tmp_objects);
        return 0;
    }
    *objects = tmp_objects;

    return 1;
}

void parallel_fitness_function(sack_object *objects, individual *generation, int sack_capacity, int start, int end) {
    int weight;
    int profit;

    for (int i = start; i < end; ++i) {
        weight = 0;
        profit = 0;

        for (int j = 0; j < generation[i].chromosome_length; ++j) {
            if (generation[i].chromosomes[j]) {
                weight += objects[j].weight;
                profit += objects[j].profit;
            }
        }
        generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
    }
}

void print_objects(const sack_object *objects, int object_count) {
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit) {
    for (int i = 0; i < limit; ++i) {
        for (int j = 0; j < generation[i].chromosome_length; ++j) {
            printf("%d ", generation[i].chromosomes[j]);
        }
        printf("\n%d - %d\n", i, generation[i].fitness);
    }
}

void print_best_fitness(const individual *generation) {
    printf("%d\n", generation[0].fitness);
}

void copy_individual(const individual *from, const individual *to) {
    memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void first_30_elite(thread_data *tdata, int count) {
    int start = tdata->thread_id * (double) count / tdata->P;
    int end = min((tdata->thread_id + 1) * (double) count / tdata->P, count);

    for (int i = start; i < end; ++i) {
        copy_individual(tdata->current_generation + i, tdata->next_generation + i);
    }
}

void first_generation(individual *current_generation, individual *next_generation, int object_count) {
    for (int i = 0; i < object_count; ++i) {
        current_generation[i].fitness = 0;
        current_generation[i].chromosomes = (int *) calloc(object_count, sizeof(int));
        current_generation[i].chromosomes[i] = 1;
        current_generation[i].index = i;
        current_generation[i].chromosome_length = object_count;

        next_generation[i].fitness = 0;
        next_generation[i].chromosomes = (int *) calloc(object_count, sizeof(int));
        next_generation[i].index = i;
        next_generation[i].chromosome_length = object_count;
    }
}

void mutate_bit_string_1(const individual *ind, int generation_index) {
    int i, mutation_size;
    int step = 1 + generation_index % (ind->chromosome_length - 2);

    if (ind->index % 2 == 0) {
        // for even-indexed individuals, mutate the first 40% chromosomes by a given step
        mutation_size = ind->chromosome_length * 4 / 10;
        for (i = 0; i < mutation_size; i += step) {
            ind->chromosomes[i] = 1 - ind->chromosomes[i];
        }
    } else {
        // for even-indexed individuals, mutate the last 80% chromosomes by a given step
        mutation_size = ind->chromosome_length * 8 / 10;
        for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i = i + step) {
            ind->chromosomes[i] = 1 - ind->chromosomes[i];
        }
    }
}

void parallel_mbs1(thread_data *tdata, int cursor, int index, int count) {
    int start = tdata->thread_id * (double) count / tdata->P;
    int end = min((tdata->thread_id + 1) * (double) count / tdata->P, count);

    for (int i = start; i < end; ++i) {
        copy_individual(tdata->current_generation + i, tdata->next_generation + cursor + i);
        mutate_bit_string_1(tdata->next_generation + cursor + i, index);
    }
}

void mutate_bit_string_2(const individual *ind, int generation_index) {
    int step = 1 + generation_index % (ind->chromosome_length - 2);

    // mutate all chromosomes by a given step
    for (int i = 0; i < ind->chromosome_length; i = i + step) {
        ind->chromosomes[i] = 1 - ind->chromosomes[i];
    }
}

void parallel_mbs2(thread_data *tdata, int cursor, int index, int count) {
    int start = tdata->thread_id * (double) count / tdata->P;
    int end = min((tdata->thread_id + 1) * (double) count / tdata->P, count);

    for (int i = start; i < end; ++i) {
        copy_individual(tdata->current_generation + i + count, tdata->next_generation + cursor + i);
        mutate_bit_string_2(tdata->next_generation + cursor + i, index);
    }
}

void crossover(individual *parent1, individual *child1, int generation_index) {
    individual *parent2 = parent1 + 1;
    individual *child2 = child1 + 1;
    int count = 1 + generation_index % parent1->chromosome_length;

    memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
    memcpy(child1->chromosomes + count, parent2->chromosomes + count,
           (parent1->chromosome_length - count) * sizeof(int));

    memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
    memcpy(child2->chromosomes + count, parent1->chromosomes + count,
           (parent1->chromosome_length - count) * sizeof(int));
}

int cmpfunc(const void *a, const void *b) {
    int i;
    individual *first = (individual *) a;
    individual *second = (individual *) b;

    int res = second->fitness - first->fitness; // decreasing by fitness
    if (res == 0) {
        int first_count = 0, second_count = 0;

        for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
            first_count += first->chromosomes[i];
            second_count += second->chromosomes[i];
        }

        res = first_count - second_count; // increasing by number of objects in the sack
        if (first_count - second_count == 0) {
            return second->index - first->index;
        }
    }

    return res;
}

// merge sort algorithm
void merge_sort(individual *generation, int left, int right) {
    int middle = left + (right - left) / 2;
    if (left < right) {
        merge_sort(generation, left, middle);
        merge_sort(generation, middle + 1, right);
        merge(generation, left, middle, right);
    }
}

// merges 2 sorted parts
void merge(individual *generation, int left, int middle, int right) {
    int i, j, k;
    int left_length = middle - left + 1;
    int right_length = right - middle;
    individual *left_array = malloc(left_length * sizeof(individual));
    individual *right_array = malloc(right_length * sizeof(individual));

    // build the left array
    for (i = 0; i < left_length; ++i) {
        left_array[i] = generation[left + i];
    }
    // build the right array
    for (i = 0; i < right_length; ++i) {
        right_array[i] = generation[middle + 1 + i];
    }

    i = j = 0;
    k = left;
    // compare elements and choose the smaller one from the 2 arrays
    while (i < left_length && j < right_length) {
        if (cmpfunc(&left_array[i], &right_array[j]) <= 0) {
            generation[k++] = left_array[i++];
        } else {
            generation[k++] = right_array[j++];
        }
    }

    // add the remaining elements to the original array
    while (i < left_length) {
        generation[k++] = left_array[i++];
    }
    while (j < right_length) {
        generation[k++] = right_array[j++];
    }

    free(left_array);
    free(right_array);
}

void parallel_merge_sort(individual *generation, thread_data *tdata) {
    int thread_elements = tdata->object_count / tdata->P;
    // each thread gets an equal part of the array
    int left = tdata->thread_id * thread_elements;
    int right = (tdata->thread_id + 1) * thread_elements - 1;
    // add remaining elements to last thread
    if (tdata->thread_id == tdata->P - 1) {
        right += (tdata->object_count % tdata->P);
    }
    // apply the algorithm to parts of the array
    merge_sort(tdata->current_generation, left, right);
}

// merging the sorted parts of the array
// group_count -> how many groups are left in the array
// group_cluster -> how many sorted parts are in a group (starts at 1)
void merge_array(individual *generation, thread_data *tdata, int group_count, int group_cluster) {
    // number of elements per thread
    int thread_elements = tdata->object_count / tdata->P;
    int group_size = thread_elements * group_cluster;
    for (int i = 0; i < group_count; i += 2) {
        // get 2 consecutive sorted parts
        int left = i * group_size;
        int right = (i + 2) * group_size - 1;
        int middle = left + (right - left + 1) / 2 - 1;
        // out of array bounds
        if (right >= tdata->object_count || left < 0) {
            right = tdata->object_count - 1;
        }
        merge(generation, left, middle, right);
    }
    // merge until we have only one part left (the original fully sorted array)
    // takes into account the case where there is an even number of groups using =
    if (group_count >= 2) {
        merge_array(generation, tdata, group_count / 2, group_cluster * 2);
    }
}

void free_generation(individual *generation) {
	for (int i = 0; i < generation->chromosome_length; i++) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}