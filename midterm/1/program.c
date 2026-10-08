#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>

#ifndef MAX_COUNTER
#define MAX_COUNTER (1 << 20 )
#endif
#ifndef VIRTUAL_THREADS
#define VIRTUAL_THREADS 2
#endif
#ifndef INITIAL_THREADS
#define INITIAL_THREADS 2
#endif
static const unsigned char INITIAL_N[] = {0xFF};
#define INITIAL_N_SIZE sizeof(INITIAL_N)
#define INITIAL_SHA1 {0}

pthread_barrier_t barrier;

typedef struct GroupData {
    int group_id;
    unsigned char last_sha1[SHA_DIGEST_LENGTH];
    unsigned char partner_last_sha1[SHA_DIGEST_LENGTH];
    int partner_id;
    int counter_start;
    int counter_end;
    struct GroupData *all_groups; // Self-referential pointer to the full array
} GroupData;

void single_thread_simulation(int groups, GroupData *group_data) {
    unsigned char input[sizeof(int) + SHA_DIGEST_LENGTH + sizeof(int) + INITIAL_N_SIZE];
    unsigned char output[SHA_DIGEST_LENGTH];

    for (int step = 0; step < MAX_COUNTER; ++step) {
        for (int i = 0; i < groups; ++i) {
            memcpy(input, &group_data[i].group_id, sizeof(int));
            memcpy(input + sizeof(int), group_data[i].partner_last_sha1, SHA_DIGEST_LENGTH);
            memcpy(input + sizeof(int) + SHA_DIGEST_LENGTH, &step, sizeof(int));
            memcpy(input + sizeof(int) + SHA_DIGEST_LENGTH + sizeof(int), INITIAL_N, INITIAL_N_SIZE);

            SHA1(input, sizeof(input), output);
            memcpy(group_data[i].last_sha1, output, SHA_DIGEST_LENGTH);
        }

        for (int i = 0; i < groups; ++i) {
            memcpy(group_data[group_data[i].partner_id].partner_last_sha1, group_data[i].last_sha1, SHA_DIGEST_LENGTH);
        }
    }
}

void group_function(GroupData *group) {
    unsigned char input[sizeof(int) + SHA_DIGEST_LENGTH + sizeof(int) + INITIAL_N_SIZE];
    unsigned char output[SHA_DIGEST_LENGTH];

    for (int step = 0; step < MAX_COUNTER; ++step) {
        memcpy(input, &group->group_id, sizeof(int));
        memcpy(input + sizeof(int), group->partner_last_sha1, SHA_DIGEST_LENGTH);
        memcpy(input + sizeof(int) + SHA_DIGEST_LENGTH, &step, sizeof(int));
        memcpy(input + sizeof(int) + SHA_DIGEST_LENGTH + sizeof(int), INITIAL_N, INITIAL_N_SIZE);

        SHA1(input, sizeof(input), output);
        memcpy(group->last_sha1, output, SHA_DIGEST_LENGTH);

        pthread_barrier_wait(&barrier);
       
        memcpy(group->all_groups[group->partner_id].partner_last_sha1, group->last_sha1, SHA_DIGEST_LENGTH);
    
        pthread_barrier_wait(&barrier);
    }
}

void *thread_wrapper(void *arg) {
    GroupData *group = (GroupData *)arg;
    group_function(group);
    return NULL;
}

void initialize_groups(int groups, GroupData *group_data, unsigned char *initial_sha1) {
    for (int i = 0; i < groups; ++i) {
        group_data[i].group_id = i;
        group_data[i].counter_start = 0;
        group_data[i].counter_end = MAX_COUNTER;
        memcpy(group_data[i].last_sha1, initial_sha1, SHA_DIGEST_LENGTH);
        memcpy(group_data[i].partner_last_sha1, initial_sha1, SHA_DIGEST_LENGTH);
        group_data[i].all_groups = group_data;
    }
}

void assign_partners(int groups, GroupData *group_data) {
    for (int i = 0; i < groups; i += 2) {
        group_data[i].partner_id = i + 1;
        group_data[i + 1].partner_id = i;
    }
}

void create_threads(int groups, pthread_t *threads, GroupData *group_data) {
    for (int i = 0; i < groups; ++i) {
        if (pthread_create(&threads[i], NULL, thread_wrapper, &group_data[i]) != 0) {
            fprintf(stderr, "Error: Unable to create thread %d\n", i);
            exit(1);
        }
    }
}

void join_threads(int groups, pthread_t *threads) {
    for (int i = 0; i < groups; ++i) {
        pthread_join(threads[i], NULL);
    }
}

int main(int argc, char *argv[]) {
    int groups = INITIAL_THREADS; // Number of threads
    if (INITIAL_THREADS == 1) {
        // Single-threaded simulation
        groups = VIRTUAL_THREADS; // Use the defined number of virtual threads
    }
    GroupData group_data[groups];
    pthread_t threads[groups];
    unsigned char initial_sha1[SHA_DIGEST_LENGTH] = INITIAL_SHA1;
    unsigned char final_sha1[SHA_DIGEST_LENGTH] = {0};

    if (INITIAL_THREADS > 1) {
        pthread_barrier_init(&barrier, NULL, groups);
    }

    initialize_groups(groups, group_data, initial_sha1);
    assign_partners(groups, group_data);
    printf("Number of threads: %d\n", INITIAL_THREADS);
    printf("MAX_COUNTER: %d\n", MAX_COUNTER);
    if (INITIAL_THREADS == 1) {
        single_thread_simulation(groups, group_data);
    } else {
        create_threads(groups, threads, group_data);
        join_threads(groups, threads);
        pthread_barrier_destroy(&barrier);
    }

    for (int i = 0; i < groups; ++i) {
        for (int j = 0; j < SHA_DIGEST_LENGTH; ++j) {
            final_sha1[j] ^= group_data[i].last_sha1[j];
        }
    }

    printf("\nFinal SHA1: ");
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        printf("%02x", final_sha1[i]);
    }
    printf("\n");

    return 0;
}

