override CFLAGS := -Wall -Werror -std=gnu99 -O0 -g $(CFLAGS) -I.
CC = gcc

all: test_busy_threads

# Build the threads.o file                                                                                   
threads.o: threads.c ec440threads.h

# build the busy_threads.o file
busy_threads.o: ./tests/busy_threads.c ec440threads.h
	$(CC) -c ./tests/busy_threads.c

# make executable                                                                                            
test_busy_threads: busy_threads.o threads.o
	$(CC) busy_threads.o threads.o -o test_busy_threads

full_threads.o: ./tests/full_threads.c ec440threads.h
	$(CC) -c ./tests/full_threads.c
test_full_threads: full_threads.o threads.o
	$(CC) full_threads.o threads.o -o test_full_threads

makemore.o: ./tests/makemore.c ec440threads.h
	$(CC) -c ./tests/makemore.c
test_makemore: makemore.o ec440threads.h
	$(CC) makemore.o threads.o -o test_makemore

test4.2.o: ./tests/test4.2.c ec440threads.h
	$(CC) -c ./tests/test4.2.c
test_test4.2: test4.2.o ec440threads.h
	$(CC) test4.2.o threads.o -o test_test4.2

test.o: ./tests/test.c ec440threads.h
	$(CC) -c ./tests/test.c
test: test.o threads.o ec440threads.h
	$(CC) test.o threads.o -o test
test_files=./test_busy_threads ./test_full_threads ./test_makemore ./test_test4.2 ./test
# test_files = ./test_test4.2

.PHONY: clean check checkprogs all

# Build all of the test programs                                                                             
checkprogs: $(test_files)

# Run the test programs                                                                                      
check: checkprogs
	/bin/bash tests/run_tests.sh $(test_files)

clean:
	rm -f *.o $(test_files) $(test_o_files)
	rm -rf thread_test_folder