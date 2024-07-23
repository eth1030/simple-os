#ifndef TLS_H_
#define TLS_H_
#include <pthread.h>

#define MAX_THREAD_COUNT 128
// creates local storage area for currently-exectuing thread
int tls_create(unsigned int size);
// frees LSA of current thread
int tls_destroy();
// reads data from TLS
int tls_read(unsigned int offset, unsigned int length, char *buffer);
// writes data to current TLS
int tls_write(unsigned int offset, unsigned int length, const char *buffer);
// clones local storage area of target thread
int tls_clone(pthread_t tid);

#endif /* TLS_H_ */
