////////////////////////////////////////////////////////////////////////////////
// Main File:        my_magic_square.c
// This File:        my_magic_square.c
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure that represents a magic square
typedef struct {
    int size;           // dimension of the square
    int **magic_square; // ptr to 2D heap array that stores magic square values
} MagicSquare;

/* Get valid magic square size from user
 * Pre-conditions: None
 * Post-conditions: Returns valid odd integer >= 3
 * retval: Valid magic square size
 */
int getSize() {
    int n;
    printf("Enter magic square's size (odd integer >=3)\n");
    if (scanf("%d", &n) != 1) {
        printf("Invalid input\n");
        exit(1);
    }
    
    if (n < 3) {
        printf("Magic square size must be >= 3.\n");
        exit(1);
    }
    if (n % 2 == 0) {
        printf("Magic square size must be odd.\n");
        exit(1);
    }
    return n;
}

/* Generate magic square using Siamese method
 * Pre-conditions: n is odd integer >= 3
 * Post-conditions: Returns initialized MagicSquare struct
 * n - the number of rows and columns
 * returns: Pointer to completed MagicSquare struct
 */
MagicSquare *generateMagicSquare(int n) {
    // Allocate MagicSquare struct
    MagicSquare *ms = malloc(sizeof(MagicSquare));
    if (ms == NULL) {
        perror("malloc failed");
        exit(1);
    }
    ms->size = n;

    // Allocate 2D array
    ms->magic_square = malloc(n * sizeof(int *));
    if (ms->magic_square == NULL) {
        free(ms);
        perror("malloc failed");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        *(ms->magic_square + i) = malloc(n * sizeof(int));
        if (*(ms->magic_square + i) == NULL) {
            for (int j = 0; j < i; j++) free(*(ms->magic_square + j));
            free(ms->magic_square);
            free(ms);
            perror("malloc failed");
            exit(1);
        }
        memset(*(ms->magic_square + i), 0, n * sizeof(int));
    }

    // Siamese method implementation
    int row = 0, col = n / 2;
    for (int num = 1; num <= n * n; num++) {
        *(*(ms->magic_square + row) + col) = num;
        int next_row = (row - 1 + n) % n;
        int next_col = (col + 1) % n;

        if (*(*(ms->magic_square + next_row) + next_col) == 0) {
            row = next_row;
            col = next_col;
        } else {
            row = (row + 1) % n;
        }
    }

    return ms;
}

/* Write magic square to file in specified format
 * Pre-conditions: magic_square is valid, filename is writable
 * Post-conditions: File created with magic square values
 * magic_square - the magic square to write
 * filename - name of output file
 */
void fileOutputMagicSquare(MagicSquare *magic_square, char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("fopen failed");
        exit(1);
    }

    // Write size header
    fprintf(fp, "%d\n", magic_square->size);

    // Write matrix rows
    for (int i = 0; i < magic_square->size; i++) {
        for (int j = 0; j < magic_square->size; j++) {
            fprintf(fp, "%d", *(*(magic_square->magic_square + i) + j));
            if (j != magic_square->size - 1) fprintf(fp, ",");
        }
        fprintf(fp, "\n");
    }

    if (fclose(fp) != 0) {
        perror("fclose failed");
        exit(1);
    }
}

/* Main program execution
 * Pre-conditions: Valid output filename provided
 * Post-conditions: Magic square generated and written to file
 */
int main(int argc, char **argv) {
    // Check command line arguments
    if (argc != 2) {
        printf("Usage: %s <output_filename>\n", argv[0]);
        exit(1);
    }

    // Get valid size from user
    int size = getSize();

    // Generate magic square
    MagicSquare *ms = generateMagicSquare(size);
    if (ms == NULL) {
        fprintf(stderr, "Failed to generate magic square\n");
        exit(1);
    }

    // Write to file
    fileOutputMagicSquare(ms, argv[1]);

    // Cleanup memory
    for (int i = 0; i < size; i++) free(*(ms->magic_square + i));
    free(ms->magic_square);
    free(ms);

    return 0;
}

