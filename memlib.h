#include <unistd.h>
#include <stdbool.h>

void *mem_sbrk(int incr);
void mem_reset_brk(void); 
void *mem_heap_lo(void);
void *mem_heap_hi(void);
size_t mem_heapsize(void);
size_t mem_pagesize(void);

#define ALIGNMENT 16
size_t align(size_t sz);
bool is_aligned(char *p);

void mem_init(void);               
void mem_deinit(void);

