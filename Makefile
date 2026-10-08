CC ?= cc
CFLAGS ?= -O2 -std=gnu11 -Wall -Wextra
all: hash-pairs classifier integral
hash-pairs: midterm/1/program.c
	$(CC) $(CFLAGS) $< -o $@ -lcrypto -pthread
classifier: midterm/2/main.c
	$(CC) $(CFLAGS) $< -o $@ -pthread
integral: midterm/3/multithreading.c
	$(CC) $(CFLAGS) -DTOTAL_ITERATIONS=1048576 $< -o $@ -lm -pthread
test:
	$(CC) $(CFLAGS) -DMAX_COUNTER=32 midterm/1/program.c -o hash-test -lcrypto -pthread
	$(CC) $(CFLAGS) -DMAX_COUNTER=32 -DINITIAL_THREADS=1 midterm/1/program.c -o hash-serial -lcrypto -pthread
	$(CC) $(CFLAGS) -DARRAY_SIZE=16 -DINTEGRAL_ITERATIONS=256 -DTOTAL_ITERATIONS=32 midterm/3/multithreading.c -o integral-test -lm -pthread
	$(CC) $(CFLAGS) -DARRAY_SIZE=16 -DINTEGRAL_ITERATIONS=256 -DTOTAL_ITERATIONS=32 -DNUM_THREADS=1 midterm/3/multithreading.c -o integral-serial -lm -pthread
	$(MAKE) classifier
	python3 tests/regression.py
.PHONY: all test
