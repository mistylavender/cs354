////////////////////////////////////////////////////////////////////////////////
// Main File:        p3Heap.c
// This File:        p3Heap.c
// Other Files:      p3Heap.h, test201.c, test213.c, test121.c, test110.c,
//                   test214_immedcoal.c, test123.c, test122.c, test105.c,
//                   test212_immedcoal.c, test202.c
// Semester:         CS 354 Lecture 001 Spring 2025
// Grade Group:      gg2
// Instructor:       Mahmood
//
// Author:           Kaeya Kapoor
// Email:            nkapoor5@wisc.edu
// CS Login:         nkapoor
//
//////////////////////////////////// WORK LOG  /////////////////////////////////
// 03/10: Implemented best-fit allocation with block splitting
// 03/12: Added immediate coalescing for free_block()
// 03/14: Verified alignment requirements and error handling
//////////////////// REQUIRED -- OTHER SOURCES OF HELP /////////////////////////
// Persons:          None
// Online sources:   None
// AI chats:         None
//////////////////////////// 80 columns wide ///////////////////////////////////

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include "p3Heap.h"

/* Structure representing heap block metadata
 * size_status: Combines block size and status flags using bitmask:
 * - Bits 2-31: Block size (multiple of 8 bytes, excluding header)
 * - Bit 1:     Previous block allocation status (1 = allocated)
 * - Bit 0:     Current block allocation status (1 = allocated)
 */
typedef struct blockHeader {
    int size_status;  // Size and status bitmask
} blockHeader;

/* Global pointer to first block in the heap */
blockHeader *heap_start = NULL;

/* Total size of allocated heap region including padding */
int alloc_size;

/* Allocate memory block using best-fit placement policy
 *
 * Pre-conditions:
 * - init_heap() must have been successfully called
 * - size > 0
 *
 * Parameters:
 *   size: Requested payload size in bytes (must be positive)
 *
 * Returns:
 *   8-byte aligned payload pointer, NULL if allocation fails
 */
void* alloc(int size) {
    // Validate input parameters
    if (size <= 0) {
        fprintf(stderr, "Error: alloc(%d) - Invalid size request\n", size);
        return NULL;
    }

    blockHeader *best_fit = NULL;
    size_t best_fit_size = SIZE_MAX;
    size_t best_padding = 0;

    // Traverse heap to find best fit
    blockHeader *current = heap_start;
    while (current->size_status != 1) { // Until END_MARK
        size_t current_size = current->size_status & ~0x3;
        int is_free = !(current->size_status & 1);

        if (is_free) {
            // Calculate required alignment padding
            size_t payload_start = (size_t)current + sizeof(blockHeader);
            size_t padding = (8 - (payload_start % 8)) % 8;

            // Calculate total required space (header + padding + payload)
            size_t required = sizeof(blockHeader) + padding + size;
            required = (required + 7) & ~7; // Round up to nearest multiple of 8

            // Track smallest suitable block
            if (current_size >= required && required < best_fit_size) {
                best_fit = current;
                best_fit_size = required;
                best_padding = padding;
            }
        }
        // Move to next block using current block size
        current = (blockHeader*)((char*)current + current_size);
    }

    if (!best_fit) {
        fprintf(stderr, "Error: alloc(%d) - No suitable block found\n", size);
        return NULL;
    }

    size_t old_size = best_fit->size_status & ~0x3;
    size_t remaining = old_size - best_fit_size;

    // Update current block header
    best_fit->size_status = best_fit_size | 1; // Mark as allocated
    best_fit->size_status |= (best_fit->size_status & 2); // Preserve previous status

    // Split block if remaining space is sufficient
    if (remaining >= 8) {
        // Create new free block after allocated space
        blockHeader *new_block = (blockHeader*)((char*)best_fit + best_fit_size);
        new_block->size_status = remaining | 2; // prev-alloc=1

        // Set footer for new free block
        blockHeader *footer = (blockHeader*)((char*)new_block + remaining - sizeof(blockHeader));
        footer->size_status = remaining;
    } else {
        // Use entire block without splitting
        best_fit->size_status = old_size | 1;
    }

    // Update next block's previous-allocated bit
    blockHeader *next_block = (blockHeader*)((char*)best_fit +
                           (best_fit->size_status & ~0x3));
    if (next_block->size_status != 1) { // Not END_MARK
        next_block->size_status |= 2; // Set prev-alloc=1
    }

    return (char*)best_fit + sizeof(blockHeader) + best_padding;
}

/* Free memory block with immediate coalescing
 *
 * Pre-conditions:
 * - ptr points to valid allocated payload
 * - ptr is 8-byte aligned
 * - ptr is within heap boundaries
 *
 * Parameters:
 *   ptr: Pointer to payload of block to free
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int free_block(void *ptr) {
    // Validate input parameters
    if (ptr == NULL) {
        fprintf(stderr, "Error: free_block() - NULL pointer\n");
        return -1;
    }

    // Verify 8-byte alignment
    if ((uintptr_t)ptr % 8 != 0) {
        fprintf(stderr, "Error: free_block(%p) - Misaligned pointer\n", ptr);
        return -1;
    }

    // Convert payload pointer to block header
    blockHeader *block = (blockHeader*)((char*)ptr - sizeof(blockHeader));

    // Validate block is within heap boundaries
    if ((void*)block < (void*)heap_start ||
        (void*)block >= (void*)heap_start + alloc_size) {
        fprintf(stderr, "Error: free_block(%p) - Pointer outside heap\n", ptr);
        return -1;
    }

    // Check for double free
    if ((block->size_status & 1) == 0) {
        fprintf(stderr, "Error: free_block(%p) - Double free detected\n", ptr);
        return -1;
    }

    size_t block_size = block->size_status & ~0x3;
    block->size_status &= ~1; // Mark as free

    // Add footer for freed block
    blockHeader *footer = (blockHeader*)((char*)block + block_size - sizeof(blockHeader));
    footer->size_status = block_size;

    // Coalesce with previous block if free
    if ((block->size_status & 2) == 0) { // Previous block is free
        blockHeader *prev_footer = (blockHeader*)((char*)block - sizeof(blockHeader));
        size_t prev_size = prev_footer->size_status;
        blockHeader *prev_block = (blockHeader*)((char*)block - prev_size);

        // Merge blocks
        prev_block->size_status += block_size;
        footer->size_status = prev_block->size_status;
        block = prev_block;
        block_size = prev_block->size_status;
    }

    // Coalesce with next block if free
    blockHeader *next_block = (blockHeader*)((char*)block + block_size);
    if (next_block->size_status != 1 && (next_block->size_status & 1) == 0) {
        size_t next_size = next_block->size_status & ~0x3;
        block->size_status += next_size;

        // Update merged block's footer
        footer = (blockHeader*)((char*)block + block->size_status - sizeof(blockHeader));
        footer->size_status = block->size_status;

        // Update next-next block's p-bit
        blockHeader *next_next = (blockHeader*)((char*)block + block->size_status);
        if (next_next->size_status != 1) {
            next_next->size_status &= ~2;
        }
    }

    // Update next block's p-bit
    next_block = (blockHeader*)((char*)block + block_size);
    if (next_block->size_status != 1) {
        next_block->size_status &= ~2;
    }

    return 0;
}

/* Initialize heap memory region
 *
 * Pre-conditions:
 * - Must be called exactly once before allocations
 * - sizeOfRegion > 0
 *
 * Parameters:
 *   sizeOfRegion: Minimum heap size to allocate
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int init_heap(int sizeOfRegion) {
    static int allocated_once = 0;
    int pagesize, padsize;
    void* mmap_ptr;
    int fd;

    // Validate initialization state
    if (allocated_once) {
        fprintf(stderr, "Error: init_heap() - Heap already initialized\n");
        return -1;
    }

    // Validate requested size
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error: init_heap(%d) - Invalid size request\n", sizeOfRegion);
        return -1;
    }

    // Calculate page-aligned heap size
    pagesize = getpagesize();
    padsize = (pagesize - (sizeOfRegion % pagesize)) % pagesize;
    alloc_size = sizeOfRegion + padsize;

    // Map memory region using mmap
    if ((fd = open("/dev/zero", O_RDWR)) < 0) {
        fprintf(stderr, "Error: init_heap() - Failed to open /dev/zero\n");
        return -1;
    }

    if ((mmap_ptr = mmap(NULL, alloc_size, PROT_READ|PROT_WRITE,
                        MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
        fprintf(stderr, "Error: init_heap() - mmap failed\n");
        close(fd);
        return -1;
    }

    allocated_once = 1;
    close(fd);

    // Adjust size for end marker
    alloc_size -= 8;

    // Initialize first block header with 4-byte offset for alignment
    heap_start = (blockHeader*)((char*)mmap_ptr + 4);
    heap_start->size_status = alloc_size | 2; // Mark prev-alloc=1

    // Set end marker at heap end
    blockHeader *end_mark = (blockHeader*)((char*)heap_start + alloc_size);
    end_mark->size_status = 1;

    // Initialize first block footer
    blockHeader *footer = (blockHeader*)((char*)heap_start + alloc_size - 4);
    footer->size_status = alloc_size;

    return 0;
}

/* Display heap block information
 *
 * Pre-conditions:
 * - init_heap() must have been successfully called
 */
void disp_heap() {
    int counter = 1;
    blockHeader *current = heap_start;

    printf("\n********************************** HEAP BLOCKS **********************************\n");
    printf("%-4s %-8s %-8s %-12s %-12s %s\n",
           "No.", "Status", "Prev", "Start", "End", "Size");

    while (current->size_status != 1) {
        size_t size = current->size_status & ~0x3;
        char status[8], prev_status[8];

        // Decode status bits
        snprintf(status, 8, "%s", (current->size_status & 1) ? "alloc" : "free");
        snprintf(prev_status, 8, "%s", (current->size_status & 2) ? "alloc" : "free");

        // Calculate block boundaries
        char *start = (char*)current;
        char *end = start + size - 1;

        printf("%-4d %-8s %-8s %p - %p %4zu\n",
               counter++, status, prev_status, start, end, size);

        current = (blockHeader*)((char*)current + size);
    }
    printf("********************************************************************************\n");
}
