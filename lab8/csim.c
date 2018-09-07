/*
 *
 * Student Name:Zhang Zhengtong
 * Student ID:516030910024
 *
 */
#include "cachelab.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

//define the type
typedef struct cache_line{
	long tag;
	int valid;
	int atime;
}line_t;
typedef struct cache_set{
	line_t* lines;
}set_t;

#define cache_m 64
#define MAX_FILENAME 255
#define GET_SET(x) (((x)>>cache_b) & ((1<<cache_s) -1))
#define GET_TAG(x) (((x) >> (cache_b+cache_s)) & ((1L<<cache_t)-1))

int verbose = 0, curtime =0;
int cache_s, cache_S, cache_E, cache_b, cache_t;
int hits = 0, misses = 0, evictions = 0;
set_t* sets;

void help(char* a){
	//printf("%s help\n", a);
	printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", a);
	printf("Options:\n");
	printf("  -h         Print this help message.\n");
	printf("  -v         Optional verbose flag.\n");
	printf("  -s <num>   Number of set index bits.\n");
	printf("  -E <num>   Number of lines per set.\n");
	printf("  -b <num>   Number of block offset bits.\n");
	printf("  -t <file>  Trace file.\n");
	printf("Examples:\n");
	printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", a);
	printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", a);
	exit(0);
};

void parse_command(int argc, char** argv, char* fn)
{
	//printf("func: parse_c\n");
	int result;
	cache_s = -1;
	cache_E = -1;
	cache_b = -1;
	memset(fn, 0, MAX_FILENAME+1);
	opterr=0;
	
	while((result = getopt(argc, argv, "hvs:E:b:t:")) != -1){
		switch(result){
			case 'h':
				help(argv[0]);
			case 'v':
				verbose = 1;
				break;
			case 's':
				cache_s = atoi(optarg);
				break;
			case 'E':
				cache_E = atoi(optarg);
				break;
			case 'b':
				cache_b = atoi(optarg);
				break;
			case 't':
				strncpy(fn, optarg, MAX_FILENAME);
				break;
			default:
				break;
		}
	}

	if(cache_s == -1 || cache_E == -1 || cache_b ==-1 || *fn == 0){
		printf("%s: Missing required command line argument\n",argv[0]);
		help(argv[0]);
	}
	printf("parse res: cache_s=%d, cache_E=%d, cache_b=%d\n",cache_s,cache_E,cache_b);
	printf("	    filename=%s\n", fn);
};

void init_cache()
{
	printf("func: init_c\n");
	sets = (set_t*)malloc(cache_S * sizeof (line_t));
	for(int i = 0; i < cache_S; i++){
		sets[i].lines = (line_t*)malloc(cache_E * sizeof(line_t));
		for(int j = 0; j < cache_E; j++){
			sets[i].lines[j].valid = 0;
			sets[i].lines[j].atime = 0;
		}
	}
};

void free_cache()
{
	printf("func: free_c\n");
};
void sim_instr(long addr)
{
	printf("func: sim_instr\n");
	curtime++;
	int the_set = GET_SET(addr);
	int the_tag = GET_TAG(addr);
	printf("the_set: %d, the_tag: %d\n", the_set, the_tag);
	int invalid_line = -1;
	int min_atime = 0x7fffffff, min_atime_line = 0;
	line_t* the_line;

	for(int i =0; i < cache_E; i++){
		the_line = &sets[the_set].lines[i];
		//whether hit
		if(the_line->valid && the_line->tag == the_tag){
			hits++;
			if(verbose)
				printf("hit:%d\n", hits);
			the_line->atime = curtime;
			return;
		}
		if(invalid_line == -1){
			if(!the_line->valid)
				invalid_line = i;
			else if(the_line->atime < min_atime){
				min_atime = the_line->atime;
				min_atime_line = i;
			}
		}
	}
	misses++;
	if(verbose){
		printf("miss: %d\n", misses);
	}
	if(invalid_line != -1){
		the_line = &sets[the_set].lines[invalid_line];
		the_line->valid = 1;
		the_line->tag = the_tag;
		the_line->atime = curtime;
	}
	else{
		evictions++;
		if(verbose)
			printf("eviction\n");
		the_line = &sets[the_set].lines[min_atime_line];
		the_line->tag = the_tag;
		the_line->atime = curtime;
	}


}

void sim_file(char* fn)
{	
	printf("func: sim_file\n");
	char op;//operations: L S M
	long addr, size;
	int result;

	FILE* fin = fopen(fn, "r");
	
	while(!feof(fin)){
		result = fscanf(fin, "%c %lx,%ld", &op, &addr, &size);
		if(result != 3)
			continue;
		printf("after scanf:op:%c, addr:%ld, size:%ld\n",op, addr, size);
		printf("result=%d\n", result);
		if(verbose&&(op == 'L'||op == 'S' || op == 'M'))
			printf("%c, 0x%lx, %ld\n", op, addr, size);
		switch(op){
			case 'L':
			case 'S':
				sim_instr(addr);
				break;
			case 'M'://M equals to L + S
				sim_instr(addr);
				sim_instr(addr);
				break;
			default:
				break;
		}

		if(verbose && (op == 'L' || op == 'S' || op == 'M'))
			printf("\n");
	}
	fclose(fin);
}
int main(int argc, char* argv[])
{
   if(argc == 1)
	 	help(argv[0]);
	char filename[MAX_FILENAME+1];
	parse_command(argc,argv,filename);
	cache_S = 1<<cache_s;
	cache_t = cache_m - cache_b - cache_s;
	init_cache(filename);
	sim_file(filename);
	printSummary(hits, misses, evictions);
	free_cache();
    return 0;
}
