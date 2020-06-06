all: test clean

test: threadPool.o
	gcc -ggdb -Wall -o run test.c threadPool.o osqueue.o -lpthread

threadPool.o: osqueue.o
	gcc -ggdb -Wall -c threadPool.c

osqueue.o:
	gcc -ggdb -Wall -c osqueue.c

clean:
	rm -f *.o
