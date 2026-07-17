#ifndef MYALLOC_H
#define MYALLOC_H

#include <stddef.h>

void *mymalloc(size_t size);
void myfree(void *ptr);
void *mycalloc(size_t a, size_t size);
void *myrealloc(void *ptr, size_t new_size);
void print_heap(void); // Still useful for debugging

#endif