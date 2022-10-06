.PHONY: clean

all: rendezvous

%.o : %.c
	gcc -c -g -Wall $< -o $@

rendezvous: sync.o errors.o output.o rendezvous.o
	gcc -Wall -g -o $@ $^ -lpthread

clean:
	rm -f rendezvous *.o *~
