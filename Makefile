all : sim4

sim4 : sim4.c circuitreader.c
	gcc -o $@ $^ -O3 -g -lm

clean :
	rm -rf *.o *~ sim4

