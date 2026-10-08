# Parallel Computing Coursework

Three C/pthreads midterm experiments by Amirali Moghadasi. Portable Make targets and small deterministic checks make the recovered work runnable on Linux or Cygwin.

## Build and test

Requires GCC, Make, pthreads, OpenSSL development headers and Python 3:

```bash
make
make test
```

| Executable | Experiment |
|---|---|
| `hash-pairs` | Parallel paired SHA-1 computation with barrier synchronization |
| `classifier input.txt` | Classifies whitespace-delimited tokens into number/letter/mixed JSONL streams |
| `integral` | Parallel numerical calculation; `--table` prints the result table |

The classifier truncates its output files on each run and processes the complete input, including long rows and a missing final newline. JSON strings are escaped. Worker output order can vary; token contents and counts are preserved.

The integral program replaces the original inner summation by its algebraically equivalent sum, avoiding repeated redundant work. The default executable uses `TOTAL_ITERATIONS=1048576`; the original billion-iteration workload is not run by default. Compile-time macros (`MAX_COUNTER`, `INITIAL_THREADS`, `ARRAY_SIZE`, `INTEGRAL_ITERATIONS`, `TOTAL_ITERATIONS`, `NUM_THREADS`) allow controlled benchmark sizes.

## Validation

Serial and parallel hashes are compared, classification is checked against an independent Python reference for 12,001 tokens, and serial/parallel integral tables are compared with the original numerical formula to a tolerance of 1e-10. These tests establish correctness on small fixtures; they do not report a hardware-independent speedup.
