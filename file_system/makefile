override CFLAGS := -Wall -Werror -std=gnu99 -O0 -g $(CFLAGS) -I.
CC = gcc

all: test_fs

# Build the disk.o file                                                                                   
disk.o: disk.c disk.h

# build the fs.o file
fs.o: fs.c fs.h

# test file
test.o: ./tests/test.c disk.h fs.h
	$(CC) -c ./tests/test.c

# make executable
test_fs: disk.o test.o fs.o
	$(CC) disk.o test.o fs.o -o test_fs

test_files= ./test_fs

.PHONY: clean check checkprogs all

# Build all of the test programs                                                                             
checkprogs: $(test_files)

# Run the test programs                                                                                      
check: checkprogs
	/bin/bash tests/run_tests.sh $(test_files)

clean:
	rm -f *.o $(test_files) $(test_o_files)
	rm fs1