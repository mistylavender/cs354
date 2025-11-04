////////////////////////////////////////////////////////////////////////////////
// Main File:        my_div0_handler.c
// This File:        my_div0_handler.c
// Other Files:      my_c_signal_handler.c, send_signal.c
// Semester:         CS 354 Lecture 002 Spring 2025
// Grade Group:      gg2
// Instructor:       Mahmood
//
// Author:           Kaeya Kapoor
// Email:            nkapoor5@wisc.edu
// CS Login:         nkapoor
//
////////////////////////////////////////////////////////////////////////////////
// Persons:          None
// Online sources:   None
// AI chats:         None
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

// Global counter for successful divisions
volatile sig_atomic_t success_count = 0;

/* Handle SIGFPE signal (division by zero): Prints error and exits
 *
 * Pre-conditions: Signal handler registered via sigaction(SIGFPE)
 *
 * @param sig  Signal number (expected to be SIGFPE)
 */
void handle_fpe(int sig) {
    printf("Error: a division by 0 operation was attempted.\n");
    printf("Total number of operations completed successfully: %d\n", success_count);
    printf("The program will be terminated.\n");
    exit(EXIT_SUCCESS);
}

/* Handle SIGINT signal (Ctrl+C): Prints success count and exits.
 *
 * Pre-conditions: Signal handler registered via sigaction(SIGINT)
 *
 * @param sig  Signal number (expected to be SIGINT)
 */
void handle_int(int sig) {
    printf("\nTotal number of operations completed successfully: %d\n", success_count);
    printf("The program will be terminated.\n");
    exit(EXIT_SUCCESS);
}

/* Main function: Reads integers, performs division, and handles signals.
 *
 * @return  Exit status (0 on success, non-zero on failure).
 */
int main() {
    struct sigaction sa_fpe, sa_int;

    // Register SIGFPE handler
    memset(&sa_fpe, 0, sizeof(sa_fpe));
    sa_fpe.sa_handler = handle_fpe;
    if (sigaction(SIGFPE, &sa_fpe, NULL) == -1) {
        perror("sigaction SIGFPE");
        exit(EXIT_FAILURE);
    }

    // Register SIGINT handler
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = handle_int;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    char buffer[100];
    int int1, int2;

    while (1) {
        printf("Enter first integer: ");
        fflush(stdout);
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        int1 = atoi(buffer);

        printf("Enter second integer: ");
        fflush(stdout);
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        int2 = atoi(buffer);

        // Perform division (may trigger SIGFPE)
        int quotient = int1 / int2;
        int remainder = int1 % int2;

        success_count++;
        printf("%d / %d is %d with a remainder of %d\n", int1, int2, quotient, remainder);
    }

    return 0;
}
