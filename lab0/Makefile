# NAME: Ethan Wong
# EMAIL: ethanwong@g.ucla.edu
# UID: 305319001


# Use gcc compiler with flags that allow for most compiler
# warnings and debug info, along with optimization
CC = gcc
CFLAGS = -Wall -Wextra -g -o

# Settings used by default to build the executable - uses above compiler settings/flags
default: 
	$(CC) $(CFLAGS) lab0 lab0.c

# Settings used to run the smoke-test cases within 'tests' file
check:
	./tests

# Settings to delete created files and return to original state
clean:
	rm -rf lab0 *.o *~ *.tar.gz

# Settings used to create distribution tarball
dist:
	tar -zvcf lab0-305319001.tar.gz Makefile lab0.c README tests #breakpoint.png backtrace.png

