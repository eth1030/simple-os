#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void *printMessage(void *arg) {
    printf("test_main_exit_early) start_routine executed: PASSED\n");
    sleep(2);
    printf("\ntest_main_exit_early) start_routine kept executing: PASSED\n");
    return NULL;
}

int main (int argc, char **argv) {
    pthread_t tid;
    int err = pthread_create(&tid, NULL, printMessage, NULL);
    printf("test_main_exit_early) main runs after creation: PASSED\n");
    pthread_exit(NULL);
    printf("test_main_exit_early) main exited: FAILED\n");
    return 1;
}