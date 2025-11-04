////////////////////////////////////////////////////////////////////////////////
// Main File:       check_sudoku_board.c
// This File:        check_sudoku_board.c
// Other Files:      None
// Semester:         CS 354 Lecture 002 Spring 2025
// Grade Group:      gg2
// Instructor:       Mahmood
// 
// Author:           Kaeya Kapoor
// Email:            nkapoor5@wisc.edu
// CS Login:         nkapoor
//
// Person:           None
// AI:               None
// Online sources:   None
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Delimiter character for parsing board rows
char *DELIM = ",";  

/* Check if Sudoku board has valid row/column configuration
 * Pre-conditions:
 * - board points to a size x size 2D array of integers
 * - size is between 1 and 9 inclusive
 * Post-conditions:
 * - Returns 1 if all rows/columns contain valid Sudoku values with no duplicates
 * - Returns 0 if any row/column contains invalid duplicates
 * 
 * board: Heap-allocated 2D array representing Sudoku board
 * size:  Dimension of board (n x n where 1 <= n <= 9)
 * retval: 1 for valid configuration, 0 for invalid
 */
int valid_sudoku_board(int **board, int size) {
    // Validate rows for duplicates
    for (int i = 0; i < size; i++) {
        // Frequency array for numbers 1-size (index 0 unused)
        int *freq = (int *)malloc((size + 1) * sizeof(int));
        if (freq == NULL) return 0;
        
        // Initialize frequency counters to zero
        for (int k = 0; k <= size; k++) {
            *(freq + k) = 0;
        }
        
        // Check each element in row
        for (int j = 0; j < size; j++) {
            int val = *(*(board + i) + j);
            if (val != 0) {
                // Detect duplicate non-zero values
                if (*(freq + val) > 0) {
                    free(freq);
                    return 0;
                }
                *(freq + val) = 1;
            }
        }
        free(freq);
    }

    // Validate columns using same approach as rows
    for (int j = 0; j < size; j++) {
        int *freq = (int *)malloc((size + 1) * sizeof(int));
        if (freq == NULL) return 0;
        
        for (int k = 0; k <= size; k++) {
            *(freq + k) = 0;
        }
        
        for (int i = 0; i < size; i++) {
            int val = *(*(board + i) + j);
            if (val != 0) {
                if (*(freq + val) > 0) {
                    free(freq);
                    return 0;
                }
                *(freq + val) = 1;
            }
        }
        free(freq);
    }
    return 1;
}

/* Read board size from first line of file
 * Pre-conditions:
 * - fptr is a valid open file pointer
 * - size points to valid memory location
 * Post-conditions:
 * - size contains integer value from first line of file
 * - File pointer advances to second line
 * 
 * fptr:  Valid file pointer for input file
 * size:  Pointer to store board dimension (1-9)
 */
void get_board_size(FILE *fptr, int *size) {
    // Buffer for reading first line
    char *line = NULL;
    // Length buffer for getline
    size_t len = 0;                             
    
    if (getline(&line, &len, fptr) == -1) {
        free(line);
        exit(1);
    }

    // Extract first token from line
    char *size_chars = strtok(line, DELIM);  
    *size = atoi(size_chars);
    free(line);
}

/* Main program execution
 * Pre-conditions:
 * - Exactly one command line argument provided
 * - Input file exists and is readable
 * Post-conditions:
 * - Prints "valid" or "invalid" based on board configuration
 * - Returns 0 on successful execution
 */
int main(int argc, char *argv[]) {
    // Validate command line arguments
    if (argc != 2) {
        printf("Usage: %s <input_filename>\n", argv[0]);
        exit(1);
    }

    // Input file handle
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("invalid\n");
        exit(0);
    }

    // Board dimension (1-9)
    int size;
    get_board_size(file, &size);
    
    // Validate board size before proceeding
    if (size < 1 || size > 9) {
        printf("invalid\n");
        fclose(file);
        exit(0);
    }

    // Dynamically allocated board storage
    int **board = (int **)malloc(size * sizeof(int *));
    if (board == NULL) {
        fclose(file);
        exit(1);
    }

    // Validation flag (1=valid, 0=invalid)
    int valid = 1;
    // Buffer for reading file lines
    char *line = NULL;
    // Length buffer for getline
    size_t len = 0;  
    
    // Read and parse board rows
    for (int i = 0; i < size; i++) {
        if (getline(&line, &len, file) == -1) {
            valid = 0;
            break;
        }

        // Allocate row storage
        *(board + i) = (int *)malloc(size * sizeof(int));
        if (*(board + i) == NULL) {
            // Cleanup prior allocations
            for (int k = 0; k < i; k++) free(*(board + k));
            free(board);
            free(line);
            fclose(file);
            exit(1);
        }

        // Tokenize comma-separated values
        char *token = strtok(line, DELIM);
        for (int j = 0; j < size; j++) {
            if (token == NULL) {
                valid = 0;
                break;
            }
            
            // Convert token to integer
            char *endptr;
            long num = strtol(token, &endptr, 10);
            
            // Check for conversion failure
            if (endptr == token) {
                valid = 0;
                break;
            }
            
            // Verify remaining characters are whitespace
            const char *p = endptr;
            while (*p != '\0') {
                if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
                    valid = 0;
                    break;
                }
                p++;
            }
            if (!valid) break;

            // Check numeric range
            if (num < 0 || num > size) {
                valid = 0;
                break;
            }

            // Store value using pointer arithmetic
            *(*(board + i) + j) = (int)num;
            token = strtok(NULL, DELIM);
        }

        if (token != NULL || !valid) {
            valid = 0;
            break;
        }
    }

    // Check for extra lines after board data
    if (valid) {
        while (getline(&line, &len, file) != -1) {
            valid = 0;
            break;
        }
    }

    // Cleanup file resources
    free(line);
    fclose(file);

    // Perform Sudoku validation if format is correct
    if (valid) {
        valid = valid_sudoku_board(board, size);
    }

    // Free board memory
    for (int i = 0; i < size; i++) {
        free(*(board + i));
    }
    free(board);

    printf(valid ? "valid\n" : "invalid\n");
    return 0;
}
