////////////////////////////////////////////////////////////////////////////////
// Main File:        p3Heap.c
// This File:        p3Heap.c
// Other Files:      p3Heap.h, test101.c, test102.c, test103.c, test104.c,
//                   test105.c, test110.c, test121.c, mytest.c
// Semester:         CS 354 Lecture 001 Spring 2025
// Grade Group:      gg 2
// Instructor:       Mahmood
//
// Author:           Kaeya Kapoor
// Email:            nkapoor5@wisc.edu
// CS Login:         nkapoor
//
// Persons:          None
// Online sources:   None
// AI chats:         None
////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include "p3Heap.h"

typedef struct blockHeader {
    int size_status;
} blockHeader;

blockHeader *heap_start = NULL;
int alloc_size;

/*
 * Allocate memory block using best-fit placement policy
 *
 * Pre-conditions:
 * - init_heap() must have been successfully called
 * - size > 0
 *
 * Parameters:
 *   size: Requested payload size in bytes (must be positive)
 *
 * Returns:
 *   Pointer to allocated payload (8-byte aligned), NULL if allocation fails
 */
void* alloc(int size) {
    // Validate input
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
        size_t current_size = current->size_status & ~0x3; // Mask out status bits
        int is_free = !(current->size_status & 1); // Check allocation bit

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
        // Move to next block
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
    best_fit->size_status |= (best_fit->size_status & 2); // Preserve previous block status

    // Split block if remaining space is sufficient
    if (remaining >= 8) {
        // Create new free block after allocated space
        blockHeader *new_block = (blockHeader*)((char*)best_fit + best_fit_size);
        new_block->size_status = remaining | 0; // Free block, prev-alloc=0

        // Set footer for new free block (last 4 bytes)
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

    // Return payload address (after header and padding)
    return (char*)best_fit + sizeof(blockHeader) + best_padding;
}

/*
 * Free memory block with immediate coalescing
 * Implemented to pass coalescing tests
 */
int free_block(void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "Error: free_block() - NULL pointer\n");
        return -1;
    }

    if ((uintptr_t)ptr % 8 != 0) {
        fprintf(stderr, "Error: free_block(%p) - Misaligned pointer\n", ptr);
        return -1;
    }

    // Get block header from payload pointer
    blockHeader *block = (blockHeader*)((char*)ptr - sizeof(blockHeader));

    // Verify block is within heap bounds
    if ((void*)block < (void*)heap_start ||
        (void*)block >= (void*)heap_start + alloc_size) {
        fprintf(stderr, "Error: free_block(%p) - Pointer outside heap\n", ptr);
        return -1;
    }

    // Verify block is within heap bounds
    if ((void*)block < (void*)heap_start ||
        (void*)block >= (void*)heap_start + alloc_size) {
        fprintf(stderr, "Error: free_block(%p) - Pointer out of heap range\n", ptr);
        return -1;
    }

    // Check if block is already free
    if ((block->size_status & 1) == 0) {
        fprintf(stderr, "Error: free_block(%p) - Double free detected\n", ptr);
        return -1;
    }

    size_t block_size = block->size_status & ~0x3;
    block->size_status &= ~1; // Mark as free

    // Add footer for free block
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
        footer = (blockHeader*)((char*)block + block_size + next_size - sizeof(blockHeader));
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

/*
 * Initialize heap memory region
 *
 * Pre-conditions:
 * - Must be called exactly once before any allocations
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

    // Map memory region
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

    // Initialize first block header (4-byte offset for alignment)
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

/*
 * Display heap block information
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
