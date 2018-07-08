all:
	gcc -std=c11 -w -pthread server.c -o homeworkServer -lm
	gcc -std=c11 -w -pthread client.c -o clientApp -lm

clean:
	rm -rf *.out
	rm -rf *.o
	rm -rf homeworkServer clientApp
	rm -rf *~

