all:	prog1

prog1: transfProg.c
	gcc transfProg.c -o transfProg -lpthread 
clean:
	rm -rf transfProg
