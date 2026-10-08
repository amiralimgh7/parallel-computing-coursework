#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

#ifndef NUM_THREADS
#define NUM_THREADS 12
#endif
#ifndef ARRAY_SIZE
#define ARRAY_SIZE 256
#endif
#ifndef INTEGRAL_ITERATIONS
#define INTEGRAL_ITERATIONS (1 << 16)
#endif
#ifndef TOTAL_ITERATIONS
#define TOTAL_ITERATIONS (1 << 30)
#endif
#define LOWER_LIMIT 0
#define UPPER_LIMIT M_PI
#define NUM_SUBDIVISIONS INTEGRAL_ITERATIONS

// Shared array
double results[ARRAY_SIZE];

// Mutex for synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Structure for thread data
typedef struct {
    int start;
    int end;
    double dx;
} PrecomputeData;

// Precompute worker function
void *precompute_worker(void *arg) {
    PrecomputeData *data = (PrecomputeData *)arg;
    double dx = data->dx;
    for (int m = data->start; m <= data->end; m++) {
        double integral_result = 0.0;
        double harmonic = 0.0;
        for (int j = 1; j <= m; ++j) harmonic += 1.0 / j;
        for (int i = 0; i < NUM_SUBDIVISIONS; i++) {
            double x = LOWER_LIMIT + dx * (i + 0.5);
            double summation_term = m * (sin(x * x) / (1 + x * x))
                                    + harmonic * exp(-100 * x) * cos(x * x);
            integral_result += summation_term * dx;
        }
        results[m] = integral_result;
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    (void)argv;
    pthread_t threads[NUM_THREADS];
    PrecomputeData thread_data[NUM_THREADS];

    double dx = (UPPER_LIMIT - LOWER_LIMIT) / NUM_SUBDIVISIONS;
    int range_per_thread = ARRAY_SIZE / NUM_THREADS;

    // Create threads to compute f(x) for 0 to 255
    for (int i = 0; i < NUM_THREADS; ++i) {
        thread_data[i].start = i * range_per_thread;
        thread_data[i].end = (i == NUM_THREADS - 1) ? (ARRAY_SIZE - 1) : ((i + 1) * range_per_thread - 1);
        thread_data[i].dx = dx;
        if (pthread_create(&threads[i], NULL, precompute_worker, &thread_data[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    // Join threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Perform TOTAL_ITERATIONS random mappings
    for (int i = 0; i < TOTAL_ITERATIONS; ++i) {
        int random_index = rand() % ARRAY_SIZE;
        double output = results[random_index];
        (void)output; // Suppress unused variable warning
    }

    // Print all elements in the results array for check 
  /*  printf("Results array:\n");
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        printf("results[%d] = %f\n", i, results[i]);
    }
*/
    if (argc > 1) for (int i = 0; i < ARRAY_SIZE; ++i) printf("%d %.12f\n", i, results[i]);
    printf("Computation complete.\n");
    return 0;
}

