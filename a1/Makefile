CFLAGS := -g -Wall -Werror 

all: a1-tester

stack.o: stack.c
	gcc $(CFLAGS) -c -o stack.o stack.c

queue.o: queue.c
	gcc $(CFLAGS) -c -o queue.o queue.c

a1-tester: a1-tester.o stack.o queue.o malloc.o
	g++ $(CFLAGS) -o a1-tester a1-tester.o stack.o queue.o malloc.o

clean:
	rm -rf a1-tester stack.o queue.o