PROGS=myshell
OBJS=myshell.o

all: $(PROGS)

%.o: %.c
	gcc -g -pthread -c -o $@ $<

myshell: $(OBJS)
	gcc -g -pthread -o $@ $^

clean:
	rm -rf $(PROGS) *.o

tapper:
	gcc -g tapper.c -o tapper 

observe:
	gcc -o observe observe.c bufferLib.c

reconstruct:
	gcc -o reconstruct reconstruct.c bufferLib.c

tapplot:
	gcc -o tapplot tapplot.c bufferLib.c