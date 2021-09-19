
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "myHeap.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {           

    int size_status;
    /*
     * Size of the block is always a multiple of 8.
     * Size is stored in all block headers and in free block footers.
     *
     * Status is stored only in headers using the two least significant bits.
     *   Bit0 => least significant bit, last bit
     *   Bit0 == 0 => free block
     *   Bit0 == 1 => allocated block
     *
     *   Bit1 => second last bit 
     *   Bit1 == 0 => previous block is free
     *   Bit1 == 1 => previous block is allocated
     * 
     * End Mark: 
     *  The end of the available memory is indicated using a size_status of 1.
     * 
     * Examples:
     * 
     * 1. Allocated block of size 24 bytes:
     *    Allocated Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 25
     *      If the previous block is allocated p-bit=1 size_status would be 27
     * 
     * 2. Free block of size 24 bytes:
     *    Free Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 24
     *      If the previous block is allocated p-bit=1 size_status would be 26
     *    Free Block Footer:
     *      size_status should be 24
     */
} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heapStart = NULL;     

/* Size of heap allocation padded to round to nearest page size.
 */
int allocsize;

/*
 * Additional global variables may be added as needed below
 */

 
/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block (payload) on success.
 * Returns NULL on failure.
 *
 * This function must:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8 
 *   and possibly adding padding as a result.
 *
 * - Use BEST-FIT PLACEMENT POLICY to chose a free block
 *
 * - If the BEST-FIT block that is found is exact size match
 *   - 1. Update all heap blocks as needed for any affected blocks
 *   - 2. Return the address of the allocated block payload
 *
 * - If the BEST-FIT block that is found is large enough to split 
 *   - 1. SPLIT the free block into two valid heap blocks:
 *         1. an allocated block
 *         2. a free block
 *         NOTE: both blocks must meet heap block requirements 
 *       - Update all heap block header(s) and footer(s) 
 *              as needed for any affected blocks.
 *   - 2. Return the address of the allocated block payload
 *
 * - If a BEST-FIT block found is NOT found, return NULL
 *   Return NULL unable to find and allocate block for desired size
 *
 * Note: payload address that is returned is NOT the address of the
 *       block header.  It is the address of the start of the 
 *       available memory for the requesterr.
 *
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* myAlloc(int size) {     
int blockSize;
int memoryAvailable = -1;
int trueSize;
int previousBlock = 0;
blockHeader *traverse = heapStart;
blockHeader *block = NULL;
if ((size <= 0)) { //Checks if the size is zero or negative
    return NULL;
}
size = sizeof(blockHeader) + size;
if (size % 8 != 0) { //Checks if padding is required
    size = size + 8 - (size % 8);
}
if (allocsize < size) { //Checks if size is greater than heap
    return NULL;
}
while (traverse->size_status != 1) {
    blockSize = traverse->size_status;
    trueSize = blockSize;
    if (traverse->size_status & 1) {
        trueSize -= 1;
    }
    if (traverse->size_status & 2) {
        trueSize -= 2;
    }
    if (blockSize % 2 == 0) {
        if ((trueSize >= size) &&
            ((memoryAvailable == -1) || (memoryAvailable > trueSize))) {
            memoryAvailable = trueSize;
            block = traverse;
            if (traverse->size_status & 2) {
                previousBlock = 2;
            } else {
                previousBlock = 0;
            }
        }
    }
    traverse = (blockHeader *)((char *)traverse + trueSize);
}
if (memoryAvailable > size) {
    block->size_status = size + previousBlock + 1;
    blockHeader *sp = (blockHeader *)((char *)block + size);
    if (sp->size_status == 1) {
        return ((char *)block + 4);
    }
    sp->size_status = memoryAvailable - size + 2;
    return ((char *)block + 4);
} else if (memoryAvailable == size) {
    block->size_status += 1;
    blockHeader *sp = (blockHeader *)((char *)block + size);
    if (sp->size_status == 1) {
        return ((char *)block + 4);
    }
    sp->size_status += 2;
    return ((char *)block + 4);
}
return NULL;
} 
 
/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 */                    
int myFree(void *ptr) {    
if (ptr == NULL) {
    return -1;
}
if (((unsigned int)ptr) % 8 != 0) { //Checks padding
    return -1;
}
if ((unsigned int)heapStart > (unsigned int)ptr) { //Checks if the pointer initialized is smaller than the first block of memory
    return -1;
}
if (((blockHeader *)((char *)ptr - 4))->size_status % 2 != 1) { //Checks if the block has already been freed
    return -1;
}
blockHeader *currentBlock = (blockHeader *)((char *)ptr - 4);
(currentBlock->size_status) -= 1;
int currBlock_size = (currentBlock->size_status / 8) * 8;
blockHeader *footer =
    (blockHeader *)((char *)currentBlock + currBlock_size - 4);
footer->size_status = currBlock_size;
blockHeader *nextBlock =
    (blockHeader *)((char *)currentBlock + (currentBlock->size_status / 8) * 8);
if (nextBlock->size_status != 1) {
    nextBlock->size_status -= 2;
}

return 0;
} 
 
 

/*
 * Function for traversing heap block list and coalescing all adjacent 
 * free blocks.
 *
 * This function is used for delayed coalescing.
 * Updated header size_status and footer size_status as needed.
 */
int coalesce() {
int temporarySize = 0;

blockHeader *currentBlock = heapStart;

blockHeader *blockFooter;

while (currentBlock->size_status != 1) {
    if (currentBlock->size_status % 2 == 0) //Checks if the block is free or not
    {
        blockHeader *nextBlock =  (blockHeader *)((char *)currentBlock +  (currentBlock->size_status / 8) * 8);

        if (nextBlock->size_status % 2 == 0 && nextBlock->size_status != 1)
        {
            currentBlock->size_status += (nextBlock->size_status / 8) * 8;
            temporarySize = (currentBlock->size_status / 8) * 8;
            blockFooter = ((blockHeader *)((char *)currentBlock + temporarySize - 4));
            blockFooter->size_status = temporarySize;
        }

        if ((currentBlock->size_status) % 8 == 0) {
            int p_bit = ((blockHeader *)((char *)currentBlock - 4))->size_status;
            blockHeader *p = (blockHeader *)((char *)currentBlock - p_bit);
            p->size_status += (currentBlock->size_status / 8) * 8;
            temporarySize = (p->size_status / 8) * 8;
            blockFooter = ((blockHeader *)((char *)p + temporarySize - 4));
            blockFooter->size_status = temporarySize;
        }
    }

    currentBlock = (blockHeader *)((char *)currentBlock + (currentBlock->size_status / 8) * 8);
}

return 0; 
}

 
/* 
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int myInit(int sizeOfRegion) {    
 
    static int allocated_once = 0; //prevent multiple myInit calls
 
    int pagesize;   // page size
    int padsize;    // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* endMark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    allocsize = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    allocsize -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heapStart = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    endMark = (blockHeader*)((void*)heapStart + allocsize);
    endMark->size_status = 1;

    // Set size in header
    heapStart->size_status = allocsize;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heapStart->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heapStart + allocsize - 4);
    footer->size_status = allocsize;
  
    return 0;
} 
                  
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void dispMem() {     
 
    int counter;
    char status[6];
    char p_status[6];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout, 
	"*********************************** Block List **********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "alloc");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "FREE ");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "alloc");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "FREE ");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
    fprintf(stdout, 
	"*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4d\n", used_size);
    fprintf(stdout, "Total free size = %4d\n", free_size);
    fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
    fprintf(stdout, 
	"*********************************************************************************\n");
    fflush(stdout);

    return;  
} 


// end of myHeap.c (sp 2021)                                         

