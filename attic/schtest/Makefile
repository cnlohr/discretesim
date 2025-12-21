all : test

FILES = test.c yspice.c yspice.h

test : $(FILES)
	gcc -o $@ $(filter %.c,$(FILES))

clean :
	rm -rf 
