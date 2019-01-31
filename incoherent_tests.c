#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sched.h>

#define CACHE_LINE 64
#define PAGE_SIZE  4096
#define MEM_SZ     (5*1024*1024*1024UL)
#define MEM_ITEMS  (MEM_SZ/CACHE_LINE)
//#define LOCAL_MEM_TEST

#define full_mb()      __asm__ __volatile__ ("mfence":::"memory")

char *mem;
volatile int core_id = 0, wcache = 0;
volatile size_t sz = 1<<26;

static void
usage(void)
{
	printf("incoherent.test\n"
	" Options:\n"
	"  -c N            core id\n"
	"  -s N            testing memory size default 64M\n"
	"  -w N            0 - no cache operaion, otherwise wb/flush cache\n"
	"  -h              help\n"
	"\n");
}

static void
test_parse_args(int argc, char **argv)
{
	char opt;

	if (argc < 2) {
		usage();
		exit(-1);
	}
	while ((opt=getopt(argc, argv, "c:s:w:h:")) != EOF) {
		switch (opt) {
		case 'c':
			core_id = atoi(optarg);
			break;
		case 's':
			sz = atoi(optarg);
			break;
		case 'w':
			wcache = atoi(optarg);
			break;
		case 'h':
			usage();
			exit(0);
		}
	}
}

void
thd_set_affinity(pthread_t tid, int cpuid)
{
        cpu_set_t s;
        int ret;

        CPU_ZERO(&s);
        CPU_SET(cpuid, &s);

        ret = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &s);
        if (ret) {
                perror("setting affinity error\n");
                exit(-1);
        }
}

static inline void
flush_cache(void *p)
{
	__asm__ __volatile__("clflushopt (%0)" :: "r"(p));
}

static inline void
wb_cache(void *p)
{
	__asm__ __volatile__("clwb (%0)" :: "r"(p));
}

static inline void
clflush_range(void *s, size_t size)
{
	size_t i;
	for(i=0; i<size; i+=CACHE_LINE) {
		flush_cache(s);
		s += CACHE_LINE;
	}
	full_mb();
}

static inline void
clwb_range(void *s, size_t size)
{
	size_t i;
	for(i=0; i<size; i+=CACHE_LINE) {
		wb_cache(s);
		s += CACHE_LINE;
	}
	full_mb();
}

static void
validate_mem(char *m, size_t size, int core)
{
	size_t i;
	if (wcache) clflush_range(mem, size);
	for(i=0; i<size; i++) {
//		printf("i %d %c\n", i, m[i]);
		if (m[i] != '$') printf("core %d position %p %c\n", core, &m[i], m[i]);
		assert(m[i] == '$');
	}
}

static void
exec(void)
{
	thd_set_affinity(pthread_self(), core_id);
	validate_mem(mem, sz, core_id);
}

int
main(int argc, char *argv[])
{
	thd_set_affinity(pthread_self(), 0);
	test_parse_args(argc, argv);
#ifndef LOCAL_MEM_TEST
	char *file = "/lfs/cache_test";
	int fd = open(file, O_CREAT | O_RDWR, 0666);
	ftruncate(fd, MEM_SZ);
	mem = (char *)mmap(0, MEM_SZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
#else
	mem = malloc(sz);
#endif
	memset(mem, '$', sz);
	if (wcache) clwb_range(mem, sz);
	full_mb();
	validate_mem(mem, sz, 0);
	exec();

	return 0;
}

