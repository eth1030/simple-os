override CFLAGS := -Wall -Werror -std=gnu99 -pedantic -O0 -g -pthread $(CFLAGS)
override LDLIBS := -pthread $(LDLIBS)

all: test

tls.o: tls.c

test.o: ./tests/test.c ./tls.h
	$(CC) -c ./tests/test.c

test2.o: ./tests/test2.c ./tls.h
	$(CC) -c ./tests/test2.c

test3.o: ./tests/test3.c ./tls.h
	$(CC) -c ./tests/test3.c

test: test.o tls.o 

test2: test2.o tls.o

test3: test3.o tls.o

test_files = ./test3 ./test ./test2 

.PHONY: clean

checkprogs: $(test_files)

check: checkprogs
	/bin/bash tests/run_tests.sh $(test_files)

clean:
	rm -f tls.o
	rm -f *.o $(test_files) $(test_o_files)

