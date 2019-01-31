#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define _GNU_SOURCE
#include <sched.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/mman.h>

#define CACHE_LINE 64
#define PAGE_SIZE  4096
#define MEM_SZ     (1<<26) 	/* 64MB */
#define ITER       100000
#define MEM_ITEMS  (MEM_SZ/CACHE_LINE)
//#define LOCAL_MEM_TEST

struct cache_line {
	unsigned int v;
	struct cache_line *next; /* random access */
} __attribute__((aligned(CACHE_LINE)));

#ifdef LOCAL_MEM_TEST
volatile struct cache_line mem[MEM_ITEMS];
#else
volatile struct cache_line *mem;
#endif

inline uint64_t
rdtsc(void)
{
	unsigned long a, d;

	asm volatile ("rdtscp" : "=a" (a), "=d" (d) : : "ebx", "ecx");

	return ((uint64_t)d << 32) | (uint64_t)a;
}

/* in global scope to force the compiler to emit the writes */
unsigned int accum = 0;

static inline void
exec(void)
{
	int iter;
	uint64_t start, end, tot = 0;

	start = rdtsc();
	for (iter = 0 ; iter < ITER ; iter++) {
	     accum   += mem[0].v;
	}
	end   = rdtsc();
	tot  = end-start;
	assert(end-start > 0);
	printf("cost %lu iter %d avg %lu\n", tot, iter, tot/ITER);
}

int
main(void)
{
#ifndef LOCAL_MEM_TEST
	char *file = "/lfs/cache_test";
	int fd = open(file, O_CREAT | O_RDWR, 0666);
	ftruncate(fd, MEM_SZ);
	mem = (struct cache_line *)mmap(0, MEM_SZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
#endif

	exec();

	return 0;
}
