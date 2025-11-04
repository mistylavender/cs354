////////////////////////////////////////////////////////////////////////////////
// Main File:        my_c_signal_handler.c
// This File:        my_c_signal_handler.c
// Other Files:      send_signal.c, my_div0_handler.c
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
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>

// Global counter for SIGUSR1 signals
volatile sig_atomic_t usr1_count = 0;
// Alarm interval in seconds
const int SECONDS = 3;

/* Handle SIGALRM signal: Prints PID and current time, then rearms alarm
 *
 * Pre-conditions:
 *   - Signal handler registered via sigaction(SIGALRM)
 *   - Global constant SECONDS is defined for alarm interval
 *
 * @param sig  Signal number (expected to be SIGALRM)
 */
void handle_alarm(int sig) {
    pid_t pid = getpid();
    time_t now = time(NULL);
    if (now == (time_t)-1) {
        perror("time");
        exit(EXIT_FAILURE);
    }
    printf("PID: %d CURRENT TIME: %s", pid, ctime(&now));
    if (alarm(SECONDS) < 0) {
        perror("alarm");
        exit(EXIT_FAILURE);
    }
}

/* Handle SIGUSR1 signal: Increments counter and prints message
 *
 * Pre-conditions: Signal handler registered via sigaction(SIGUSR1)
 *
 * @param sig  Signal number (expected to be SIGUSR1)
 */
void handle_usr1(int sig) {
    usr1_count++;
    printf("Received SIGUSR1, user signal 1 counted.\n");
}

/* Handle SIGINT signal (Ctrl+C): Prints SIGUSR1 count and exits.
 *
 * Pre-conditions: Signal handler registered via sigaction(SIGINT)
 *
 * @param sig  Signal number (expected to be SIGINT)
 */
void handle_int(int sig) {
    printf("\nSIGINT handled.\n");
    printf("SIGUSR1 was handled %d times. Exiting now.\n", usr1_count);
    exit(EXIT_SUCCESS);
}


/* Main function: Registers signal handlers and starts infinite loop.
 *
 * @return  Exit status (0 on success, non-zero on failure).
 */
int main() {
    struct sigaction sa_alrm, sa_usr1, sa_int;

    // Register SIGALRM handler
    memset(&sa_alrm, 0, sizeof(sa_alrm));
    sa_alrm.sa_handler = handle_alarm;
    if (sigaction(SIGALRM, &sa_alrm, NULL) == -1) {
        perror("sigaction SIGALRM");
        exit(EXIT_FAILURE);
    }

    // Register SIGUSR1 handler
    memset(&sa_usr1, 0, sizeof(sa_usr1));
    sa_usr1.sa_handler = handle_usr1;
    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }

    // Register SIGINT handler
    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = handle_int;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    printf("PID and current time: prints every 3 seconds.\n");
    printf("Type Ctrl-C to end the program.\n");

    // Set initial alarm
    if (alarm(SECONDS) < 0) {
        perror("alarm");
        exit(EXIT_FAILURE);
    }

    while (1) {} // Infinite loop

    return 0;
}
