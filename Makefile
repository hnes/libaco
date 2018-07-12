TARGET = test_aco_synopsis
CC = gcc
CFLAGS = -g -O2 -Wall -Werror

test_aco_synopsis : acosw.o aco.o test_aco_synopsis.o
	cc -o test_aco_synopsis acosw.o aco.o test_aco_synopsis.o

acosw.o : acosw.S
	$(CC) $(CFLAGS) -c acosw.S -o acosw.o
aco.o : aco.c aco.h aco_assert_override.h
	$(CC) $(CFLAGS) -c aco.c -o aco.o
test_aco_synopsis.o : test_aco_synopsis.c aco.h aco_assert_override.h
	$(CC) $(CFLAGS) -c test_aco_synopsis.c -o test_aco_synopsis.o

clean :
	rm acosw.o aco.o test_aco_synopsis.o test_aco_synopsis
