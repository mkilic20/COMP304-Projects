#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

typedef struct MemoryBlock
{
    size_t size;
    struct MemoryBlock *next;
    struct MemoryBlock *prev;
    unsigned char isFree;
} MemoryBlock;

#define ALIGNMENT 8
#define SIZEOFBLOCK sizeof(MemoryBlock)
#define SIZEOFPOOL (6 * 1024)

MemoryBlock *head = NULL;
//Returns aligned size
size_t alignSize(size_t size)
{
    return (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1));
}
//Splits the large memory block into two smaller blocks. 
void splitBlock(MemoryBlock *block, size_t size)
{
    size_t remSize = block->size - size - SIZEOFBLOCK;
    if (remSize > SIZEOFBLOCK + ALIGNMENT)
    {
        MemoryBlock *newBlock = (MemoryBlock *)((char *)block + SIZEOFBLOCK + size);
        newBlock->size = remSize;
        newBlock->isFree = 1;
        newBlock->next = block->next;
        newBlock->prev = block;
        if (newBlock->next)
        {
            newBlock->next->prev = newBlock;
        }
        block->size = size;
        block->next = newBlock;
    }
}
//Checks free blocks and returns best-fit block.
MemoryBlock *findBestFitFree(MemoryBlock **last, size_t size)
{
    MemoryBlock *current = head;
    MemoryBlock *bestFit = NULL;

    while (current)
    {
        if (current->isFree && current->size >= size)
        {
            if (!bestFit || current->size < bestFit->size)
            {
                bestFit = current;
            }
        }

        *last = current;
        current = current->next;
    }

    if (bestFit)
    {
        if (bestFit->size >= size + SIZEOFBLOCK + ALIGNMENT)
        {
            splitBlock(bestFit, size);
        }
        bestFit->isFree = 0;
    }

    return bestFit;
}
//When no suitable memory block found, this function increases program’s data segment.
MemoryBlock *extendHeap(MemoryBlock *last, size_t size)
{
    size_t totalSize = SIZEOFPOOL;
    if (size + SIZEOFBLOCK > totalSize)
    {
        totalSize = alignSize(size + SIZEOFBLOCK);
    }

    MemoryBlock *block = sbrk(totalSize);
    if (block == (void *)-1)
    {
        return NULL;
    }
    block->size = size;
    block->isFree = 0;
    block->next = NULL;
    block->prev = last;

    if (last)
    {
        last->next = block;
    }

    size_t remSize = totalSize - size - SIZEOFBLOCK;
    if (remSize > SIZEOFBLOCK)
    {
        MemoryBlock *freeBlock = (MemoryBlock *)((char *)block + SIZEOFBLOCK + size);
        freeBlock->size = remSize - SIZEOFBLOCK;
        freeBlock->isFree = 1;
        freeBlock->next = NULL;
        freeBlock->prev = block;
        block->next = freeBlock;
    }

    return block;
}
//Merges adjacent memory blocks to create larger block
MemoryBlock *mergeBlocks(MemoryBlock *block)
{
    if (block->next && block->next->isFree)
    {
        block->size += SIZEOFBLOCK + block->next->size;
        block->next = block->next->next;
        if (block->next)
        {
            block->next->prev = block;
        }
    }
    if (block->prev && block->prev->isFree)
    {
        block->prev->size += SIZEOFBLOCK + block->size;
        block->prev->next = block->next;
        if (block->next)
        {
            block->next->prev = block->prev;
        }
        block = block->prev;
    }
    return block;
}
//Sets memory blocks to zero to be used in calloc
void zeroization(void *inputblock, size_t inputSize)
{
    memset(inputblock, 0, inputSize);
}
//Returns the block address from the pointer
MemoryBlock *getBlockAddress(void *ptr)
{
    return (MemoryBlock *)((char *)ptr - SIZEOFBLOCK);
}
/*
First aligns the requested size using alignSize function. If it is the first allocation 
using extendHeap, extends the heap with given size. This block becomes the head of the linked 
list. If it is not the first allocation, the function uses findBestFitFree method to find suitable 
block. If found, splits the block if necessary. Thus, allocates memory of the given size
*/
void *kumalloc(size_t size)
{
    if (size <= 0)
    {
        return NULL;
    }

    size_t correctedSize = alignSize(size);
    MemoryBlock *block;

    if (!head)
    {

        block = extendHeap(NULL, correctedSize);
        if (!block)
        {
            return NULL;
        }
        head = block;
    }
    else
    {

        MemoryBlock *last = head;
        block = findBestFitFree(&last, correctedSize);

        if (!block)
        {

            block = extendHeap(last, correctedSize);
            if (!block)
            {
                return NULL;
            }
        }
        else
        {

            block->isFree = 0;

            if (block->size > correctedSize + SIZEOFBLOCK)
            {
                splitBlock(block, correctedSize);
            }
        }
    }
    return (void *)(block + 1);
}
/*
Takes number of elements and element sizes and multiply them to find the find total 
size. After allocating total memory from the kumalloc, uses zeroization to create these elements 
from that total sized block
*/
void *kucalloc(size_t numElements, size_t elementSize)
{
    if (numElements == 0 || elementSize == 0)
    {
        return NULL;
    }

    size_t totalSize = numElements * elementSize;
    size_t correctedSize = alignSize(totalSize);
    void *newBlock = kumalloc(correctedSize);

    if (newBlock)
    {
        zeroization(newBlock, correctedSize);
    }

    return newBlock;
}
/*Takes the pointer of the data block to be freed. It gets the data block by 
getBlockAddress function, then sets its free value to 1. Then uses mergeBlocks to merge it if it applicable. If freed block is the last block in the memory pool, it uses brk to reduce the heap size.*/
void kufree(void *ptr)
{
    if (!ptr)
    {
        return;
    }

    MemoryBlock *block = getBlockAddress(ptr);
    block->isFree = 1;
    block = mergeBlocks(block);

    if (block->next == NULL && block->isFree)
    {

        if (block->prev)
        {
            block->prev->next = NULL;
        }
        else
        {
            head = NULL;
        }

        if (brk(block) == -1)
        {

            fprintf(stderr, "Reduce Error!!!\n");
        }
    }
}
/*
Takes the pointer and new size and if new size is sufficient, returns the pointer. If the 
current block smaller than the requested size using kumalloc, the function asks for a new 
sufficient block. Then copies from the old data to the new block and frees the old block using 
kufree. 
*/
void *kurealloc(void *ptr, size_t size)
{
    if (size == 0)
    {
        kufree(ptr);
        return NULL;
    }

    if (!ptr)
    {
        return kumalloc(size);
    }

    MemoryBlock *block = (MemoryBlock *)ptr - 1;
    if (block->size >= size)
    {
        return ptr;
    }

    void *newPtr = kumalloc(size);
    if (newPtr)
    {
        memcpy(newPtr, ptr, block->size);
        kufree(ptr);
    }

    return newPtr;
}

/*
 * Enable the code below to enable system allocator support for your allocator.
 * Doing so will make debugging much harder (e.g., using printf may result in
 * infinite loops).
 */
// #if 1
void *malloc(size_t size) { return kumalloc(size); }
void *calloc(size_t nmemb, size_t size) { return kucalloc(nmemb, size); }
void *realloc(void *ptr, size_t size) { return kurealloc(ptr, size); }
void free(void *ptr) { kufree(ptr); }
// #endif