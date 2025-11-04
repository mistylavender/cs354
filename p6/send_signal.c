////////////////////////////////////////////////////////////////////////////////
// Main File:        send_signal.c
// This File:        send_signal.c
// Other Files:      my_c_signal_handler.c, my_div0_handler.c
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
#include <unistd.h>

/* Main function: Sends SIGUSR1/SIGINT to a specified PID.
 *
 * Pre-conditions:
 *   - Valid PID provided as second command-line argument.
 *   - First argument must be "-u" (SIGUSR1) or "-i" (SIGINT).
 *
 * @param argc  Number of command-line arguments (must be 3)
 * @param argv  Command-line arguments array
 * @return      EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: send_signal -u <pid> to send SIGUSR1\n");
        printf("       send_signal -i <pid> to send SIGINT\n");
        return EXIT_FAILURE;
    }

    char *option = argv[1];
    if (option[0] != '-' || (option[1] != 'u' && option[1] != 'i')) {
        fprintf(stderr, "Invalid option: %s\n", option);
        return EXIT_FAILURE;
    }

    int sig = (option[1] == 'u') ? SIGUSR1 : SIGINT;
    pid_t pid = (pid_t)atoi(argv[2]);

    if (kill(pid, sig) == -1) {
        perror("kill");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
