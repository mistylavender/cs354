////////////////////////////////////////////////////////////////////////////////
// Main File: csim.c
// This File: csim.c
// Semester: CS 354 Lecture 001 Spring 2025
// Grade Group: gg2
// Instructor: Mahmood
//
// Author: Kaeya Kapoor
// Email: nkapoor5@wisc.edu
// CS Login: nkapoor
//
// Persons: None
// Online sources: None
// AI chats: None
////////////////////////////////////////////////////////////////////////////////

/**
 * csim.c:
 * Simulate a cache with LRU replacement policy and count hits, misses, and evictions.
 *
 * Implementation assumptions:
 * - LRU policy uses timestamp counters to track recency.
 * - Ignores 'I' (instruction) accesses.
 * - Handles 'M' as a load followed by a store.
 */

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/******************************************************************************/
/* DO NOT MODIFY THESE VARIABLE NAMES and TYPES                               */
/* DO UPDATE THEIR VALUES AS NEEDED BY YOUR CSIM                              */

// Globals set by command line args.
int b = 0; // number of (b) bits
int s = 0; // number of (s) bits
int E = 0; // number of lines per set

// Globals derived from command line args.
int B; // block size in bytes: B = 2^b
int S; // number of sets: S = 2^s

// Global counters to track cache statistics in access_data().
int hit_cnt = 0;
int miss_cnt = 0;
int evict_cnt = 0;

// Global to control trace output
int verbosity = 0; // print trace if set
/******************************************************************************/

// Global timestamp counter for LRU tracking
unsigned int time_stamp = 0;
typedef unsigned long long int mem_addr_t; // 64-bit memory address

/**
 * cache_line_t: Metadata for a single cache line.
 *
 * valid: 1 if the line holds data, 0 otherwise.
 * tag: Tag bits from the memory address.
 * last_used_time: Timestamp for LRU replacement (higher = more recent).
 */
typedef struct cache_line {
    char valid;
    mem_addr_t tag;
    unsigned int last_used;
} cache_line_t;

typedef cache_line_t* cache_set_t;  // A set is an array of E cache lines
typedef cache_set_t* cache_t;       // The cache is an array of S sets

cache_t cache;  // Global cache instance (dynamically allocated)

/**
 * init_cache:
 * Allocates memory for the cache and initializes all lines to invalid.
 *
 * Pre-conditions:
S (2^s) and E are derived from valid command-line arguments.
 * Post-conditions:
 * - cache[S][E] is allocated on the heap with valid=0 for all lines.
 * - Exits with error on malloc failure.
 */
void init_cache() {
    cache = (cache_set_t*)malloc(S * sizeof(cache_set_t));
    if (cache == NULL) {
        fprintf(stderr, "Error: Failed to allocate cache sets\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < S; i++) {
        cache[i] = (cache_line_t*)malloc(E * sizeof(cache_line_t));
        if (cache[i] == NULL) {
            fprintf(stderr, "Error: Failed to allocate set %d\n", i);
            exit(EXIT_FAILURE);
        }

        // Initialize all lines in the set
        for (int j = 0; j < E; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].last_used = 0;
        }
    }
}

/**
 * free_cache:
 * Releases all memory allocated for the cache.
 *
 * Pre-conditions: Cache was initialized via init_cache().
 * Post-conditions: All heap memory for the cache is freed.
 */
void free_cache() {
    if (cache == NULL) return;

    for (int i = 0; i < S; i++) {
        free(cache[i]);
    }
    free(cache);
}

/**
 * access_data:
 * Simulates a memory access at address `addr`, updating cache state and statistics.
 *
 * Pre-conditions:
 * - Cache is initialized.
 * - `addr` is a valid 64-bit memory address.
 * Post-conditions:
 * - hit_cnt, miss_cnt, or evict_cnt are incremented as needed.
 * - LRU timestamps are updated.
 *
 * @param addr The memory address being accessed.
 */
void access_data(mem_addr_t addr) {
    // Extract tag and set index from address
    mem_addr_t tag = addr >> (s + b);  // Shift out set index and block offset
    unsigned int set_index = (addr >> b) & ((1 << s) - 1); // Mask to get s bits

    cache_line_t *set = cache[set_index];  // Target set for the address

    // Check for a hit
    for (int i = 0; i < E; i++) {
        if (set[i].valid && set[i].tag == tag) {
            hit_cnt++;
            set[i].last_used = time_stamp++;  // Update LRU timestamp
            if (verbosity) printf("hit ");
            return;
        }
    }

    // Cache miss
    miss_cnt++;
    int invalid_idx = -1;

    // Find an invalid line to fill
    for (int i = 0; i < E; i++) {
        if (!set[i].valid) {
            invalid_idx = i;
            break;
        }
    }

    if (invalid_idx != -1) {
        // Fill invalid line
        set[invalid_idx].valid = 1;
        set[invalid_idx].tag = tag;
        set[invalid_idx].last_used = time_stamp++;
        if (verbosity) printf("miss ");
    } else {
        // Evict LRU line
        evict_cnt++;
        int lru_idx = 0;
        unsigned int min_time = set[0].last_used;

        // Find line with smallest timestamp (least recently used)
        for (int i = 1; i < E; i++) {
            if (set[i].last_used < min_time) {
                min_time = set[i].last_used;
                lru_idx = i;
            }
        }

        // Replace LRU line
        set[lru_idx].tag = tag;
        set[lru_idx].last_used = time_stamp++;
        if (verbosity) printf("miss eviction ");
    }
}

/**
 * replay_trace:
 * Replays the given trace file against the cache.
 *
 * Pre-conditions:
 * - `trace_fn` is a valid filename with read permissions.
 * Post-conditions:
 * - All accesses in the trace are simulated.
 * - Verbose output is printed if enabled.
 *
 * @param trace_fn Path to the trace file.
 */
void replay_trace(char* trace_fn) {
    char buf[1000];
    mem_addr_t addr = 0;
    unsigned int len = 0;
    FILE* trace_fp = fopen(trace_fn, "r");

    if (!trace_fp) {
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
        exit(1);
    }

    while (fgets(buf, 1000, trace_fp) != NULL) {
      // Skip lines that don't contain 'L', 'S', or 'M'
      if (strchr(buf, 'L') == NULL && strchr(buf, 'S') == NULL && strchr(buf, 'M') == NULL) {
          continue;
      }

      // Extract operation (ignore leading/trailing spaces)
      char op = '\0';
      mem_addr_t addr = 0;
      unsigned int len = 0;

      // Parse line for "op addr,len"
      if (sscanf(buf, " %c %llx,%u", &op, &addr, &len) != 3) {
          if (sscanf(buf, "%c %llx,%u", &op, &addr, &len) != 3) {
              continue; // Skip invalid lines
          }
      }

      // Process valid operations
      if (op != 'L' && op != 'S' && op != 'M') continue;

      if (verbosity) printf("%c %llx,%u ", op, addr, len);

      switch (op) {
          case 'L':
          case 'S':
              access_data(addr);
              break;
          case 'M':
              access_data(addr); // Load
              access_data(addr); // Store
              break;
      }
      if (verbosity) printf("\n");
    }

    fclose(trace_fp);
}

/**
 * print_usage:
 * Prints command-line usage instructions and exits.
 *
 * @param argv Command-line arguments array (for program name).
 */
void print_usage(char* argv[]) {
    printf("Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Enable verbose output.\n");
    printf("  -s <s>     Number of set index bits (S = 2^s sets).\n");
    printf("  -E <E>     Number of lines per set.\n");
    printf("  -b <b>     Number of block offset bits (B = 2^b bytes).\n");
    printf("  -t <file>  Path to trace file.\n");
    printf("\nExamples:\n");
    printf("  %s -s 4 -E 1 -b 4 -t traces/trace1\n", argv[0]);
    exit(0);
}

/**
 * print_summary:
 * Outputs simulation results to stdout and .csim_results.
 *
 * @param hits      Total cache hits.
 * @param misses    Total cache misses.
 * @param evictions Total evictions.
 */
void print_summary(int hits, int misses, int evictions) {
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}

/**
 * main: cache simulation workflow
 * 1. Parses command-line arguments
 * 2. Initializes cache
 * 3. Replays memory trace
 * 4. Cleans up resources
 * 5. Prints results
 *
 * Pre-conditions:
 * - Command-line arguments follow the format:
 *   [-hv] -s <s> -E <E> -b <b> -t <tracefile>
 * Post-conditions:
 * - Returns 0 on success, 1 on invalid arguments or errors
 * - Outputs results to stdout and .csim_results
 *
 * @param argc  Argument count (number of command-line tokens)
 * @param argv  Argument vector (array of command-line strings)
 * @return      Exit status code (0 = success, 1 = error)
 */
int main(int argc, char* argv[]) {

    char* trace_file = NULL;
    char c;

    // Parse command-line arguments
    while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        switch (c) {
            case 'b':
                b = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'h':
                print_usage(argv);
                exit(0);
            case 's':
                s = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            case 'v':
                verbosity = 1;
                break;
            default:
                print_usage(argv);
                exit(1);
        }
    }

    // Validate required arguments
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required arguments\n", argv[0]);
        print_usage(argv);
        exit(1);
    }

    // Compute derived values
    B = 1 << b;  // Block size = 2^b
    S = 1 << s;  // Number of sets = 2^s

    // Initialize and simulate
    init_cache();
    replay_trace(trace_file);
    free_cache();

    // Output results
    print_summary(hit_cnt, miss_cnt, evict_cnt);
    return 0;
}
