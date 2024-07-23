#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "../tls.h"

int main() {

  
  const char data[] = "Hellodfsnajfkndsajkf";
  const int data_len = strlen(data);
  
  const int tls_size = 10000;
  if (tls_create(tls_size) == -1) {
    return -1;
  }

  const int offset = 0;
  tls_write(offset, data_len, data);

  
  char read_buffer[data_len];

  
  tls_read(offset, data_len, read_buffer);

  // printf("%s\n",data);
  // printf("%c\n", read_buffer[data_len]);

  assert(memcmp(read_buffer, data, data_len) == 0);

  assert(strcmp(read_buffer,data) == 0);
  
  return 0;
}
