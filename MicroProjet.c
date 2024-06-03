#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

#define NUM_CHILDREN 4

sem_t *sem_start;
pid_t children[NUM_CHILDREN];

void handle_signal(int sig) {
    if (sig == SIGUSR1) {
        printf("Child %d received start signal\n", getpid());
        // Simulate complex task
        sleep(2); // Simulate task duration
        // Send confirmation signal to parent
        kill(getppid(), SIGUSR2);
    }
}

void parent_signal_handler(int sig) {
    static int confirmations = 0;
    if (sig == SIGUSR2) {
        confirmations++;
        printf("Parent received confirmation from a child. Total confirmations: %d\n", confirmations);
        if (confirmations == NUM_CHILDREN) {
            printf("All children have completed their tasks.\n");
            // Cleanup
            for (int i = 0; i < NUM_CHILDREN; i++) {
                kill(children[i], SIGTERM);
            }
            sem_unlink("/sem_start");
            sem_close(sem_start);
            exit(0);
        }
    }
}

void child_process() {
    // Set up signal handler for SIGUSR1
    signal(SIGUSR1, handle_signal);
    // Wait for the start signal from parent
    sem_wait(sem_start);
    // Pause and wait for SIGUSR1
    pause();
    exit(0);
}

int main() {
    // Set up signal handler for parent
    signal(SIGUSR2, parent_signal_handler);

    // Initialize semaphore
    sem_start = sem_open("/sem_start", O_CREAT, 0644, 0);
    if (sem_start == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Create child processes
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            child_process();
        } else {
            children[i] = pid;
        }
    }

    // Ensure all children are ready
    sleep(1); // Simulate setup time

    // Release all children to start their tasks
    for (int i = 0; i < NUM_CHILDREN; i++) {
        sem_post(sem_start);
    }

    // Send start signal to all children
    for (int i = 0; i < NUM_CHILDREN; i++) {
        kill(children[i], SIGUSR1);
    }

    // Wait for all children to complete their tasks
    for (int i = 0; i < NUM_CHILDREN; i++) {
        wait(NULL);
    }

    // Cleanup
    sem_unlink("/sem_start");
    sem_close(sem_start);

    return 0;
}
