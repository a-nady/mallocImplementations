// This file gives you a starting point to implement malloc using implicit list
// Each chunk has a header (of type header_t) and does *not* include a footer
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "mm-common.h"
#include "mm-implicit.h"
#include "memlib.h"

// turn "debug" on while you are debugging correctness. 
// Turn it off when you want to measure performance
static bool debug = false;

size_t hdr_size = sizeof(header_t);

void 
init_chunk(header_t *p, size_t csz, bool allocated)
{
	p->size = csz;
	p->allocated = allocated;
}

// Helper function next_chunk returns a pointer to the header of 
// next chunk after the current chunk h.
// It returns NULL if h is the last chunk on the heap.
// If h is NULL, next_chunk returns the first chunk if heap is non-empty, and NULL otherwise.
header_t *
next_chunk(header_t *h)
{
    header_t *p;

	if (!h) {
        p = mem_heap_lo();
        size_t c = 0;
        while (c < mem_heapsize()) {
            if (p->size) {
                return p;
            }
            p++;
            c += 2;
            if ((char *)p >= (char *)mem_heap_hi())
                return NULL;
        }
        return NULL;
	}

	p = (header_t *)((char *)h + h->size);

    if ((char *)p >= (char *)mem_heap_hi())
        return NULL;

    if (p->size)
        return p;

    return NULL;
}


/* 
 * mm_init initializes the malloc package.
 */
int mm_init(void)
{
	assert(hdr_size == align(hdr_size));
    mem_sbrk(0);

	if (!mem_heapsize())
	    return 0;

    return -1;
}


// helper function first_fit traverses the entire heap chunk by chunk from the begining. 
// It returns the first free chunk encountered whose size is bigger or equal to "csz".  
// It returns NULL if no large enough free chunk is found.
// Please use helper function next_chunk when traversing the heap
header_t *
first_fit(size_t csz)
{
    if (!mem_heapsize())
        return NULL;

    header_t *p = next_chunk(NULL);

    while (p) {
        if (!p->allocated) {
            if (p->size >= csz)
                return p;
        }
        p = next_chunk(p);
    }

    return NULL;
}

// helper function split cuts the chunk into two chunks. The first chunk is of size "csz", 
// the second chunk contains the remaining bytes. 
// You must check that the size of the original chunk is big enough to enable such a cut.
void
split(header_t *original, size_t csz)
{
    if (!original)
        return;

    if (original->size <= csz)
        return;

    size_t csz2 = original->size - csz;
    original->size = csz;

    header_t *p = (header_t *)((char *)original + csz);

    init_chunk(p, csz2, original->allocated);
}

// helper function ask_os_for_chunk invokes the mem_sbrk function to ask for a chunk of 
// memory (of size csz) from the "operating system". It initializes the new chunk 
// using helper function init_chunk and returns the initialized chunk.
header_t *
ask_os_for_chunk(size_t csz)
{
	header_t *p = (header_t*)mem_sbrk(csz);
	init_chunk(p, csz, false);
	return p;
}

/* 
 * mm_malloc allocates a memory block of size bytes
 */
void *
mm_malloc(size_t size)
{
	size = align(size);
	size_t csz = hdr_size + align(size);
	header_t *p = first_fit(csz);

    if (p) {
        split(p, csz);
    }
    else {
        p = ask_os_for_chunk(csz);
    }

    init_chunk(p, csz, true);
    p = (header_t *)((char *)p + hdr_size);

	if (debug)
		mm_checkheap(true);

	return (void *)p;
}

// Helper function payload_to_header returns a pointer to the 
// chunk header given a pointer to the payload of the chunk 
header_t *
payload2header(void *p)
{
	return (header_t *)((char *)p - hdr_size);
}

// Helper function coalesce merges free chunk h with subsequent 
// consecutive free chunks to become one large free chunk.
// You should use header2next when implementing this function
void
coalesce(header_t *h)
{
    size_t csz = 0;
    header_t *p = next_chunk(h);
    header_t *temp_p;

    while (p) {
        temp_p = next_chunk(p);

        if (p->allocated) {
            break;
        } else {
            csz += p->size;
            p->size = 0;
        }
        p = temp_p;
    }

    h->size += csz;
}

/*
 * mm_free frees the previously allocated memory block
 */
void 
mm_free(void *p)
{
	header_t *h = payload2header(p);

	if (!h->size)
	    return;

	h->allocated = false;
    coalesce(h);

	if (debug)
		mm_checkheap(true);
}	

/*
 * mm_realloc changes the size of the memory block pointed to by ptr to size bytes.  
 * The contents will be unchanged in the range from the start of the region up to the minimum of   
 * the  old  and  new sizes.  If the new size is larger than the old size, the added memory will   
 * not be initialized.  If ptr is NULL, then the call is equivalent  to  malloc(size).
 * if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr).
 */
void *
mm_realloc(void *ptr, size_t size)
{
    header_t *p2 = NULL;

	if (!ptr)
	    ptr = mm_malloc(size);

    header_t *h = payload2header(ptr);
	size_t p_csz = h->size - hdr_size;

	if (!h->allocated)
	    return ptr;

	if (!size) {
	    mm_free(ptr);
	} else if ((size + hdr_size < p_csz) && size > 0) {
        split(h, (size +  hdr_size));
        p2 = (header_t *)((char *)h + size +  hdr_size + hdr_size );
        mm_free(p2);
    } else if (size > p_csz) {
        mm_free(ptr);
        p2 = (header_t *)mm_malloc(size);
        memcpy(p2, ptr, size);
        ptr = p2;
	}

	if (debug)
		mm_checkheap(true);

	return ptr;
}


/*
 * mm_checkheap checks the integrity of the heap and returns a struct containing 
 * basic statistics about the heap. Please use helper function next_chunk when 
 * traversing the heap
 */
heap_info_t 
mm_checkheap(bool verbose) 
{

	heap_info_t info;
    info.num_allocated_chunks = 0, info.num_free_chunks = 0,
    info.allocated_size = 0, info.free_size = 0;

    header_t *p = next_chunk(NULL);
    
    while (p) {
        if (p->allocated) {
            info.num_allocated_chunks++;
            info.allocated_size += p->size;
        } else {
            info.num_free_chunks++;
            info.free_size += p->size;
        }

        if (mem_heapsize() == (info.allocated_size + info.free_size)) {
            break;
        }

        p = next_chunk(p);
    }

    if (verbose) {
        printf("\nnum_allocated_chunks = %ld\nallocated_size = %ld\nnum_free_chunks = %ld\nfree_size = %ld\nheap size = %ld\n",
               info.num_allocated_chunks, info.allocated_size, info.num_free_chunks, info.free_size, mem_heapsize());
    }

	assert(mem_heapsize() == (info.allocated_size + info.free_size));
	return info;
}
