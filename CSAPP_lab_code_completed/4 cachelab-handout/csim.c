#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "cachelab.h"

typedef struct {
    unsigned long tag;
    int valid;
    int lru;
} CacheLine;

typedef struct {
    CacheLine *lines;
} CacheSet;

typedef struct {
    CacheSet *sets;
    int set_count;
    int lines_per_set;
} Cache;

typedef struct {
    int hits;
    int misses;
    int evictions;
} CacheStats;

void initCache(Cache *cache, int s, int E) {
    int set_count = 1<<s;
    cache->sets = (CacheSet*)malloc(set_count * sizeof(CacheSet));
    cache->set_count = set_count; // rows
    cache->lines_per_set = E; // columns
    for (int i = 0; i < set_count; i++) {
        cache->sets[i].lines = (CacheLine*)malloc(E * sizeof(CacheLine));
        for (int j = 0; j < E; j++) {
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].lru = j;
        }
    }
}

void freeCache(Cache *cache) {
    for (int i = 0; i < cache->set_count; i++) {
        free(cache->sets[i].lines);
    }
    free(cache->sets);
}

void updateLRU(CacheSet *set, int lines_per_set, int accessed_index) {
    // accessed_index is the index/location of the line in one set
    for (int i = 0; i < lines_per_set; i++) {
        set->lines[i].lru++;
    }
    set->lines[accessed_index].lru = 0; // smaller number means most recently accessed
}


void accessCache(Cache *cache, CacheStats *stats, unsigned long address, int s, int b) {
    
    unsigned long tag = address >> (s+b);
    int set_index = (address >> b) & ((1<<s)-1);
    CacheSet *set = &cache->sets[set_index];

    // Check for hit
    for (int i = 0; i < cache->lines_per_set; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            stats->hits++;
            // printf("hit ");
            updateLRU(set, cache->lines_per_set, i);
            // if (op == 'M') {
            //     stats->hits++;
            //     // printf("hit");
            // }
            return;
        }
    }

    // Miss occurred for load, modify, and store
    stats->misses++;
    // printf("miss ");


    // Check for an empty line to load/write/modify
    for (int i = 0; i < cache->lines_per_set; i++) {
        if (!set->lines[i].valid) {
            set->lines[i].valid = 1;
            set->lines[i].tag = tag;
            updateLRU(set, cache->lines_per_set, i);
            // if (op == 'M') {
            //     stats->hits++;
            //     // printf("hit");
            // }
            return;
        }
    }

    // Eviction needed;
    stats->evictions++;
    // printf("eviction ");
    // Find the LRU line to evict
    int lru_index = 0;
    int lru_max = -1;
    for (int i = 0; i < cache->lines_per_set; i++) {
        if (set->lines[i].lru > lru_max) {
            lru_max = set->lines[i].lru;
            lru_index = i;
        }
    }

    set->lines[lru_index].tag = tag;
    updateLRU(set, cache->lines_per_set, lru_index);
    // if (op == 'M'){
    //     stats->hits++;
    //     // printf("hit");
    // }

}


int main(int argc, char*argv[])
{
    int opt;
    int s = 0, E = 0, b = 0;
    char *tracefile;

    // get arguments
    while ((opt = getopt(argc, argv, "s:E:b:t:")) != -1) {
        switch (opt) {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                tracefile = optarg;
                break;
            case '?':
                fprintf(stderr, "Usage: %s -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // check if all required arguments are provided
    if (tracefile == NULL || s==0 || E == 0 || b == 0) {
        fprintf(stderr, "Usage: %s -f filename -a integer1 -b integer2 -c integer3\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    Cache cache;
    initCache(&cache, s, E);

    CacheStats stats = {0, 0, 0};

    FILE *file = fopen(tracefile,"r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char operation;
    unsigned long address;
    int size;
    while (fscanf(file, " %c %lx,%d", &operation, &address, &size) == 3) {
        // fprintf(stdout, "%c %lu,%d", operation, address, size)
        if (operation == 'L' || operation == 'S' || operation == 'M') {
            accessCache(&cache, &stats, address, s, b);
            // printf("\n");
            if (operation == 'M') {
                accessCache(&cache, &stats, address, s, b); // For 'M', treat as load followed by store
            }
        }
    }
    fclose(file);

    printSummary(stats.hits, stats.misses, stats.evictions);

    freeCache(&cache);
    return 0;
}
