#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "../tls.h"

void* thread_func(void* arg) {
  int* tls_id = (int*)arg;
  tls_create(1024);
  printf("TLS %d created for thread %ld\n", *tls_id, pthread_self());
  pthread_exit(NULL);
}

int main() {
  pthread_t threads[2];
  int tls_ids[2] = {1, 2};

  // Create two threads
  for (int i = 0; i < 2; i++) {
    pthread_create(&threads[i], NULL, thread_func, (void*)&tls_ids[i]);
  }

  //Wait for threads to finish
  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);

  return 0;
}