### cache_tests.c
The test reads the same cache line of the global memory (file /lfs/cache_test) multiple times in a tight loop, 
and calculates the average cost.  
To compile it

    gcc  -O3 cache_tests.c -o mem.test

A binary file `mem.test` is generated. To run it, simply using

    ./mem.test
  
It will print out the measured results. 
For example, when I run it before on old SLES 15, the output is

    cost 95527946 iter 100000 avg 955
  
That means I access the cache line 100000 times, and the average cost is 955 cycles (cache is unlikely to be used).
### incoherent_tests.c
The program simply sets the memory to a know value on an initial core, then reads the same region out on a specific 
(possibly different) second core.  It validates that the memory has been updated to the known value.   
To compile it

     gcc incoherent_tests.c -o incoherent.test -lpthread

 This generates the file incoherent.test. To run it
 
    ./incoherent.test -c <core_id> -s <mem_sz> -w <cache_op>

<core_id> is the core it migrates to for checking the memory, <mem_sz> is the size of tested memory region, default is 64MB, 
<cache_op> is to enable (1) or disable (0) cache operations,  including write back after setting the memory, and clflush to 
invalidate memory before loads on the second core, default is 0.  
If it pass the test, it finished without printing anything. if not, it triggers an assertion and prints out the core id and 
memory location containing the unexpected data.  
One example test I got like this
```
uv4test25:~/Desktop/yuxin # ./incoherent.test -c 50 -s 1024 -w 0                        
core 50 position 0x7ffeb7819000 r                                                       
incoherent.test: incoherent_tests.c:124: validate_mem: Assertion `m[i] == '$'' failed.  
Aborted (core dumped)
uv4test25:~/Desktop/yuxin # ./incoherent.test -c 50 -s 1024 -w 1
```
Without cache operations, the test fails on core 50 for memory region is 1K. After using cache operations (-w 1), 
the test works without any assertion.  
