
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h> //debug
#include <stdbool.h>

#define Min_Size (16 + sizeof(block))


typedef struct block
{
    bool is_free;
    struct block *next;
    struct block *prev;
    size_t size;
} block;


void myfree(void *);
void coalesce(block *);
void *mymalloc(size_t);
void *myrealloc(void *, size_t);

block *heap = NULL;

void init_heap(void)
{
    size_t length = getpagesize();

    heap = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (heap == MAP_FAILED)
    {
        perror("mmap");
        return;
    }

    heap->is_free = true;
    heap->next = NULL;
    heap->prev = NULL;
    heap->size = length - sizeof(block);
}

size_t align_size(size_t a)
{
    return (a + 15) & ~15;
}

void print_heap(void)
{
    block *curr = heap;
    int i = 0;

    while (curr != NULL)
    {
        printf("Block %d\n", i++);
        printf("Address : %p\n", (void *)curr);
        printf("Size    : %zu\n", curr->size);
        printf("Free    : %s\n", curr->is_free ? "Yes" : "No");
        printf("Next    : %p\n", (void *)curr->next);
        printf("Prev    : %p\n", (void *)curr->prev);
        printf("-------------------------\n");

        curr = curr->next;
    }
}

void append(block *heap, block *next_heap, size_t length)
{

    while (heap->next != NULL)
    {
        heap = heap->next;
    }
    next_heap->is_free = true;
    next_heap->next = NULL;
    next_heap->prev = heap;
    heap->next = next_heap;
    next_heap->size = length - sizeof(block);
}

/*block* find_free_block(size_t length, block* head){

if(head == NULL) return NULL;
while(head->is_free == false){
    head = head->next;
}
if(head->size >= length){
    return head;
}
else {

    if(head->next == NULL){
    perror("allocation");
    return NULL;}
    else return find_free_block(length, head->next);
}
}*/

block *find_free_block(size_t len, block *head)
{
    block *current = head;

    while (current != NULL)
    {

        if (current->is_free == true && current->size >= len)
        {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

block *split_block(block *start, size_t length)
{

    if (start->size >= length + sizeof(block) &&
        start->size - length - sizeof(block) > Min_Size) // first check for unsigned overflow
    {

        block *new = (block *)((char *)start + sizeof(block) + length); // goated line

        new->prev = start;
        new->next = start->next;
        new->size = start->size - length - sizeof(block);
        start->size = length;
        new->is_free = true;

        if (start->next != NULL)
            start->next->prev = new;

        start->next = new;
        start->is_free = false;
        return start;
    }
    else
    {
        start->is_free = false;
        return start;
    }
}
void *mymalloc(size_t size)
{
    if (heap == NULL)
        init_heap();

    if (size == 0)
        return NULL;

    size = align_size(size);

    block *b = find_free_block(size, heap);

    if (b == NULL)
    {
        size_t len = getpagesize();
        size_t length;
        size_t needed = size + sizeof(block);
        if (needed % len != 0)
        {
            length = ((needed / len) + 1) * len;
        }
        else
            length = needed;

        block *next_heap = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (next_heap == MAP_FAILED)
        {
            perror("mmap");
            return NULL;
        }

        append(heap, next_heap, length);

        b = next_heap;
    }

    b = split_block(b, size);

    return (void *)((char *)b + sizeof(block));
}

void *my_memset(void *ptr, int value, size_t num)
{
    unsigned char *p = ptr;

    for (size_t i = 0; i < num; i++)
    {
        p[i] = (unsigned char)value;
    }

    return ptr;
}

void *mycalloc(size_t a, size_t size)
{
    if (size != 0 && a > SIZE_MAX / size)
    {
        fprintf(stderr, "Memory Overflow\n");
        return NULL;
    }
    size_t total = size * a;
    if (total == 0)
    {
        fprintf(stderr, "Memory zero\n");
        return NULL;
    }
    void *ptr = mymalloc(total);
    if (ptr != NULL)
        my_memset(ptr, 0, total);
    return ptr;
}

void *copy(void *new, void *old, size_t len)
{
    if (new == NULL || old == NULL)
    {
        perror("copyerror");
        return NULL;
    }
    unsigned char *temp = new;
    unsigned char *temp2 = old;
    if (temp < old || temp >= temp2 + len)
    {
        for (size_t i = 0; i < len; i++)
        {
            temp[i] = temp2[i];
        }
    }
    else
    {
        for (size_t i = len; i > 0; i--)
        {
            temp[i - 1] = temp2[i - 1];
        }
    }
    return new;
}

block *fix_frag(block *a, size_t len)
{
    if (a->size >= len + sizeof(block) &&
        a->size - len - sizeof(block) > Min_Size) // first check for unsigned overflow
    {

        block *new = (block *)((char *)a + sizeof(block) + len); // goated line

        new->prev = a;
        new->next = a->next;
        new->size = a->size - len - sizeof(block);
        a->size = len;
        a->next = new;
        new->is_free = true;
        a->is_free = false;
        if (new->next != NULL)
        {
            new->next->prev = new;
        }
    }
    return a;
}

void *myrealloc(void *ptr, size_t new_size)
{
    if (ptr == NULL)
    {
        return mymalloc(new_size);
    }
    else if (new_size == 0)
    {
        myfree(ptr);
        return NULL;
    }
    block *a = (block *)((char *)ptr - sizeof(block));
    new_size = align_size(new_size);
    if (new_size <= a->size)
    {
        if (a->size >= new_size + sizeof(block) + Min_Size)
            fix_frag(a, new_size);
        return ptr;
    }
    else if (a->next != NULL && a->next->is_free && a->next->size + a->size + sizeof(block) >= new_size)
    {
        coalesce(a);
        if (a->size > new_size + sizeof(block) + 16)
        {
            fix_frag(a, new_size);
            ptr = (char *)a + sizeof(block);
        }
        return ptr;
    }
    else if (a->prev != NULL && a->prev->is_free && a->prev->size + a->size + sizeof(block) >= new_size)
    {

        /* coalesce(a->prev);
         if (a->prev->size > new_size + sizeof(block) + 16)
         {
             ptr = fix_frag(a->prev, new_size);
         }
         return (void *)((char *)a->prev + sizeof(block));*/


        block *merged = a->prev;

        coalesce(merged);

        /* Move data toward the beginning */
        copy((char *)merged + sizeof(block),
             ptr,
             a->size);

        fix_frag(merged, new_size);

        return (char *)merged + sizeof(block);
    }
    else
    {

        void *newptr = mymalloc(new_size);
        void *fin_ptr = copy(newptr, ptr, a->size);
        myfree(ptr);
        return fin_ptr;
    }
}

void myfree(void *ptr)
{
    if (ptr == NULL)
        return;

    block *victim = (block *)((char *)ptr - sizeof(block));

    if (victim->is_free)
    {
        fprintf(stderr, "Double free!\n");
        return;
    }

    victim->is_free = true;
    if (victim->next != NULL && victim->next->is_free == true)
    {
        coalesce(victim);
    }
    if (victim->prev != NULL && victim->prev->is_free == true)
    {
        coalesce(victim->prev);
    }
}

void coalesce(block *a)
{

    block *b = a->next;
    if (b == NULL)
        return;

    if ((char *)a + sizeof(block) + a->size == (char *)b)
    {

        a->size += b->size + sizeof(block);
        a->next = b->next;

        if (b->next != NULL)
            b->next->prev = a;
    }
    else
    {
        fprintf(stderr, "Attempted to coalesce non-adjacent blocks\n");
        return;
    }
}

/*int main()
{
    void *a = mymalloc(100);
    void *b = mymalloc(200);
    void *c = mymalloc(300);

    print_heap();

    myfree(b);

    printf("\nAfter freeing b:\n");
    print_heap();

    myfree(a);

    printf("\nAfter freeing a:\n");
    print_heap();

    void *d = mymalloc(280);

    printf("\nAfter allocating d:\n");
    print_heap();

    return 0;
}*/