all : sim4

sim4 : sim4.c circuitreader.c
	gcc -o $@ $^ -O1 -g -lm

clean :
	rm -rf *.o *~
