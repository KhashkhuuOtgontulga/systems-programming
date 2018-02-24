/* Khashkhuu Otgontulga
 * A20379665 */

#include "cachelab.h"
/* to parse the commands */
#include <getopt.h>
/* for scanf, EXIT_SUCCESS AND EXIT_FAILURE */
#include <stdlib.h>
/* for standard I/O function prototypes */
#include <stdio.h>
/* for getopt()  */
#include <unistd.h>
/* to compute powers of 2 */
#include <math.h>

typedef unsigned long long int mem_address;
typedef long long num_sets;
typedef long long block_size;

/* structure of the cache */
struct cache {
    //int s, E, b;
    struct cache_set *sets;
  };

/* structure of the set */
struct cache_set {
    struct cache_line *lines;
  };

/* structure of the address */
struct cache_line {
    int valid;
    unsigned long tag;
    long counter;
  };

/* cache parameters */
			/* -h: optional help flag that prints usage info */
int verbose; 		/* -v: optional verbose flag that displays trace info */
int s = 0;   		/* -s <s>: number of set index bits (S = 2s is the number of sets) */
int E = 0;   		/* -E <E>: associativity (number of lines per set) */
int b = 0;   		/* -b <b>: number of block bits (B = 2b is the block size) */
char *trace_file = 0;	/* -t <tracefile>: name of the valgrind trace to replay */

/* counters */
int hit_count = 0, miss_count = 0, eviction_count = 0;
int LRU_counter = 0;

/* print help */
void usage() {
  printf("Usage: cache [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
  printf("   -h:             optional help flag that prints usage info\n");
  printf("   -v:             optional verbose flag that displays trace info\n");
  printf("   -s <s>:         Number of set index bits (S = 2^s is the number of sets)\n");
  printf("   -E <E>:         Associativity (number of lines per set)\n");
  printf("   -b <b>:         Number of block bits (B = 2^b is the block size)\n");
  printf("   -t <tracefile>: Name of the valgrind trace to replay\n");
  printf("Examples:");
  printf("   linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
  printf("   linux>  ./csim-ref -v 8 -E 2  -b 4 -t traces/yi.trace\n");
}

void parse_command(int argc, char *argv[]) {
  char c;

  /* parsing */
  while ( (c = getopt(argc,argv,"s:E:b:t:hv"))!= -1 ) {
    switch ( c ) {
	case 'h': usage(); exit(0);	break;
	case 'v': verbose = 1; 		break;
	case 's': s = atoi(optarg); 	break;
	case 'E': E = atoi(optarg); 	break;
	case 'b': b = atoi(optarg); 	break;
	case 't': trace_file = optarg;  break;
	default:  usage(); exit(0);	break;
    }
  }
   if (s == 0 || E == 0 || b == 0 || trace_file == 0) {
        printf("Error: Missing required argument\n");
        usage();
	exit(0);
  }
}

/* initialize the cache */
struct cache make_cache(num_sets S, int E, block_size B) {

  struct cache c;

  int setIndex, lineIndex;

  /* make the cache with 2^s number of sets
   * for example, s = 3 would make 8 sets in the cache
   */
  c.sets = malloc(sizeof(struct cache_set) * S);

  /* for each set in the cache, make the right amount of lines per set */
  for (setIndex = 0; setIndex < S; setIndex++) {

	c.sets[setIndex].lines = malloc(sizeof(struct cache_line) * E);

	for (lineIndex = 0; lineIndex < E; lineIndex++) {
  	    /* initialize the line */
            c.sets[setIndex].lines[lineIndex].valid = 0;
	    c.sets[setIndex].lines[lineIndex].tag = 0;
	    c.sets[setIndex].lines[lineIndex].counter = 0;
  	}
  }
  return c;
}

void free_cache(struct cache c, num_sets S, int E, block_size B) {
  int setIndex;
  /* free all the lines in each set */
  for (setIndex = 0; setIndex < E; setIndex++) {
      struct cache_set set = c.sets[setIndex];
      free(set.lines);
  }
  /* free all the sets in the cache */
  free(c.sets);
}

/* finds the least recently used */
int find_LRU(struct cache c, int setIndex) {

  int least_recently_used = 0, lineIndex;
 /* initialize the min to be the first line so that if we do not find a line with a lower counter,
  * then the first line is the least recently used */
  int  min_counter = c.sets[setIndex].lines[0].counter;

 /* given the set, search through the all the lines in that set
  * to find the least recently used line to evict */
 struct cache_set set = c.sets[setIndex];

 /* start at 1 because line.counter is already initialized so we don't have to compare
  * it to itself */
 for (lineIndex = 1; lineIndex < E; lineIndex++) {
	struct cache_line line = set.lines[lineIndex];
 	if (line.counter < min_counter) {
	        min_counter = line.counter;
		least_recently_used = lineIndex;
	}
 }
 return least_recently_used;
}

/* simulate the cache */
void simulate_cache(struct cache c, mem_address address) {
  int lineIndex = 0;

  mem_address set = (address >> b) & ((1 << s) - 1);
  mem_address tag =  address >> (s + b);

  struct cache_set selected_set = c.sets[set];

  for (lineIndex = 0; lineIndex < E; lineIndex++) {
 	struct cache_line line = selected_set.lines[lineIndex];
	/* first check if we have a hit, if so, we return because we have found a match */
	if ((line.valid == 1) && (line.tag == tag) ) {
	    hit_count++;
	    if (verbose) {
		printf("hit ");
	    }
	    LRU_counter++;
	    line.counter = LRU_counter;
	    return;
	}
  }
  /* if none of the lines match, then we know we have a miss */
  miss_count++;
  if (verbose) {
	printf("miss ");
  }
  /* we find the least recently used line for 4 reasons:
   * (1) to replace the tag of the cache line with the new memory address tag
   * (2) update the valid bit to 1 because we have data
   * (3) increase the LRU counter because we have accessed the cache
   * (4) finally update the counter of the line so we can find the least recently used line later
   */
  int lru_set = find_LRU(c, set);
  /* Notes:
   * - the valid bit of the least recently used line will always be 0 until all the lines in the cache have been filled
   * - once the cache is full, then the valid bit will be 1 in which case we can increment the eviction line and do the
   * 4 things stated above
   */
  if (selected_set.lines[lru_set].valid) {
	eviction_count++;
	if (verbose) {
		printf("eviction ");
	}
  }

  selected_set.lines[lru_set].valid = 1;
  selected_set.lines[lru_set].tag = tag;
  LRU_counter++;
  selected_set.lines[lru_set].counter = LRU_counter;
  return;
}

/* main function where we will
 * (1) parse the command
 * (2) make the cache
 * (3) read the trace file
 * (4) simulate the cache
 * (5) free the cache
 * (6) print the summary
 */
int main(int argc, char *argv[])
{
  /* (1) */
  parse_command(argc, argv);

  /* We need S, E, and B to make the cache and free the cache */
  num_sets S = pow(2, s);
  /* E = number of lines per set */
  block_size B = pow(2, b);

  /* (2)
   * initialize the cache
   * but we don't use block_size because it is not important in this lab
   */
  struct cache c = make_cache(S, E, B);

  FILE *read_trace_file;
  /* (3)
   * open the file to scan the contents of the file
   */
  read_trace_file = fopen(trace_file, "r");

  /* (4)
   * read the instruction and execute it:
   * modify(), store(), load()
   */

  /* file parameters */
  char instruction;
  mem_address address;
  int size;

  while (fscanf(read_trace_file, " %c %llx,%d", &instruction, &address, &size) == 3 ){
    switch (instruction) {
      /* L (load): read
       * S (store): write
       * same functions for the purposes in this lab
       * M (modify): read and write
       */
      case 'L':
      case 'S':
      case 'M':
		if (verbose) {
		    printf("%c %llx,%d ", instruction, address, size);
		}
		simulate_cache(c, address);
		if (instruction == 'M') {
			simulate_cache(c, address);
		}
		printf("\n");
		break;
    }
  }
  /* (5) */
  free_cache(c, S, E, B);
  /* (6) */
  printSummary(hit_count, miss_count, eviction_count);

  fclose(read_trace_file);

  return 0;
}
